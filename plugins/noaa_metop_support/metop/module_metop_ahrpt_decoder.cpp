#include "module_metop_ahrpt_decoder.h"
#include "common/codings/randomization.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/widgets/themed_widgets.h"
#include "logger.h"
#include "pipeline/module.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include <cstdint>

#define BUFFER_SIZE 8192 * 2

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace metop
{
    MetOpAHRPTDecoderModule::MetOpAHRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), d_viterbi_outsync_after(parameters["viterbi_outsync_after"].get<int>()),
          d_viterbi_ber_thresold(parameters["viterbi_ber_thresold"].get<float>()), viterbi(d_viterbi_ber_thresold, d_viterbi_outsync_after, BUFFER_SIZE)
    {
        viterbi_out = new uint8_t[BUFFER_SIZE * 2];
        soft_buffer = new int8_t[BUFFER_SIZE];
        deframer.STATE_SYNCED = 18;

        fsfsm_file_ext = ".cadu";
    }

    MetOpAHRPTDecoderModule::~MetOpAHRPTDecoderModule()
    {
        delete[] viterbi_out;
        delete[] soft_buffer;
    }

    void MetOpAHRPTDecoderModule::process()
    {
        reedsolomon::ReedSolomon rs(reedsolomon::RS223);

        uint8_t frame_buffer[1024 * 10];

        int noSyncsRuns = 0;

        while (should_run())
        {
            // Read a buffer
            read_data((uint8_t *)soft_buffer, BUFFER_SIZE);

            // Perform Viterbi decoding
            int num_samp = viterbi.work(soft_buffer, BUFFER_SIZE, viterbi_out);

            // Reconstruct into bytes and write to output file
            if (num_samp > 0)
            {
                // Deframe / derand
                int frames = deframer.work(viterbi_out, num_samp, frame_buffer);

                // Rare case where the viterbi might stay locked on
                // a bad phase at high SNR starting in the middle of a baseband
                // If it stays that way a bit too long, reset.
                if (deframer.getState() == deframer.STATE_NOSYNC)
                {
                    noSyncsRuns++;

                    if (noSyncsRuns >= 10)
                    {
                        viterbi.reset();
                        noSyncsRuns = 0;
                    }
                }
                else
                {
                    noSyncsRuns = 0;
                }

                for (int i = 0; i < frames; i++)
                {
                    uint8_t *cadu = &frame_buffer[i * 1024];

                    derand_ccsds(&cadu[4], 1024 - 4);

                    // RS Correction
                    rs.decode_interlaved(&cadu[4], true, 4, errors);

                    // Write it out
                    write_data(cadu, 1024);
                }
            }
        }

        cleanup();
    }

    nlohmann::json MetOpAHRPTDecoderModule::getModuleStats()
    {
        auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
        v["deframer_lock"] = deframer.getState() == deframer.STATE_SYNCED;
        v["viterbi_ber"] = viterbi.ber();
        v["viterbi_lock"] = viterbi.getState();
        v["rs_avg"] = (errors[0] + errors[1] + errors[2] + errors[3]) / 4;
        std::string viterbi_state = viterbi.getState() == 0 ? "NOSYNC" : "SYNCED";
        std::string deframer_state = deframer.getState() == deframer.STATE_NOSYNC ? "NOSYNC" : (deframer.getState() == deframer.STATE_SYNCING ? "SYNCING" : "SYNCED");
        v["viterbi_state"] = viterbi_state;
        v["deframer_state"] = deframer_state;
        return v;
    }

    void MetOpAHRPTDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("MetOp AHRPT Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        float ber = viterbi.ber();

        ImGui::BeginGroup();
        {
            // Constellation
            {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                ImVec2 rect_min = ImGui::GetCursorScreenPos();
                ImVec2 rect_max = {rect_min.x + 200 * ui_scale, rect_min.y + 200 * ui_scale};
                draw_list->AddRectFilled(rect_min, rect_max, style::theme.widget_bg);
                draw_list->PushClipRect(rect_min, rect_max);

                for (int i = 0; i < 2048; i++)
                {
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)soft_buffer)[i * 2 + 0] / 127.0) * 100 * ui_scale) % int(200 * ui_scale),
                                                      ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + (((int8_t *)soft_buffer)[i * 2 + 1] / 127.0) * 100 * ui_scale) % int(200 * ui_scale)),
                                               2 * ui_scale, style::theme.constellation);
                }

                draw_list->PopClipRect();
                ImGui::Dummy(ImVec2(200 * ui_scale + 3, 200 * ui_scale + 3));
            }
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Viterbi", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (viterbi.getState() == 0)
                    ImGui::TextColored(style::theme.red, "NOSYNC");
                else
                    ImGui::TextColored(style::theme.green, "SYNCED");

                ImGui::Text("BER   : ");
                ImGui::SameLine();
                ImGui::TextColored(viterbi.getState() == 0 ? style::theme.red : style::theme.green, UITO_C_STR(ber));

                std::memmove(&ber_history[0], &ber_history[1], (200 - 1) * sizeof(float));
                ber_history[200 - 1] = ber;

                widgets::ThemedPlotLines(style::theme.plot_bg.Value, "", ber_history, IM_ARRAYSIZE(ber_history), 0, "", 0.0f, 1.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
            }

            ImGui::Spacing();

            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (viterbi.getState() == 0)
                {
                    ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "NOSYNC");
                }
                else
                {
                    if (deframer.getState() == deframer.STATE_NOSYNC)
                        ImGui::TextColored(style::theme.red, "NOSYNC");
                    else if (deframer.getState() == deframer.STATE_SYNCING)
                        ImGui::TextColored(style::theme.orange, "SYNCING");
                    else
                        ImGui::TextColored(style::theme.green, "SYNCED");
                }
            }

            ImGui::Spacing();

            ImGui::Button("Reed-Solomon", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("RS    : ");
                for (int i = 0; i < 4; i++)
                {
                    ImGui::SameLine();

                    if (viterbi.getState() == 0 || deframer.getState() == deframer.STATE_NOSYNC)
                    {
                        ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "%i ", i);
                    }
                    else
                    {
                        if (errors[i] == -1)
                            ImGui::TextColored(style::theme.red, "%i ", i);
                        else if (errors[i] > 0)
                            ImGui::TextColored(style::theme.orange, "%i ", i);
                        else
                            ImGui::TextColored(style::theme.green, "%i ", i);
                    }
                }
            }
        }
        ImGui::EndGroup();

        drawProgressBar();

        ImGui::End();
    }

    std::string MetOpAHRPTDecoderModule::getID() { return "metop_ahrpt_decoder"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> MetOpAHRPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<MetOpAHRPTDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace metop