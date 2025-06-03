#include "module_fengyun_mpt_decoder.h"
#include "common/codings/randomization.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/codings/rotation.h"
#include "common/widgets/themed_widgets.h"
#include "diff.h"
#include "logger.h"
#include <cstdint>

#define BUFFER_SIZE 8192

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace fengyun3
{
    FengyunMPTDecoderModule::FengyunMPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), d_viterbi_outsync_after(parameters["viterbi_outsync_after"].get<int>()),
          d_viterbi_ber_threasold(parameters["viterbi_ber_thresold"].get<float>()), viterbi1(d_viterbi_ber_threasold, d_viterbi_outsync_after, BUFFER_SIZE, {PHASE_0, PHASE_90}),
          viterbi2(d_viterbi_ber_threasold, d_viterbi_outsync_after, BUFFER_SIZE, {PHASE_0, PHASE_90})
    {
        soft_buffer = new int8_t[BUFFER_SIZE * 2];
        q_soft_buffer = new int8_t[BUFFER_SIZE];
        i_soft_buffer = new int8_t[BUFFER_SIZE];
        viterbi1_out = new uint8_t[BUFFER_SIZE];
        viterbi2_out = new uint8_t[BUFFER_SIZE];
        diff_out = new uint8_t[BUFFER_SIZE * 20];

        fsfsm_file_ext = ".cadu";
    }

    FengyunMPTDecoderModule::~FengyunMPTDecoderModule()
    {
        delete[] soft_buffer;
        delete[] q_soft_buffer;
        delete[] i_soft_buffer;
        delete[] viterbi1_out;
        delete[] viterbi2_out;
        delete[] diff_out;
    }

    void FengyunMPTDecoderModule::process()
    {
        // Differential
        FengyunDiff diff;
        reedsolomon::ReedSolomon rs(reedsolomon::RS223);

        uint8_t frame_buffer[1024 * 10];

        // Tests
        int shift = 0;
        bool iq_invert = true;
        bool invert_branches = false;
        int noSyncRuns = 0, viterbiNoSyncRun = 0;

        while (should_run())
        {
            // Read a buffer
            read_data((uint8_t *)soft_buffer, BUFFER_SIZE * 2);

            rotate_soft(soft_buffer, BUFFER_SIZE * 2, PHASE_0, iq_invert);

            for (int i = 0; i < BUFFER_SIZE; i++)
            {
                i_soft_buffer[i] = soft_buffer[(i + shift) * 2 + 0];
                q_soft_buffer[i] = ~soft_buffer[(i + shift) * 2 + 1];
            }

            rotate_soft(i_soft_buffer, BUFFER_SIZE, PHASE_0, true);
            rotate_soft(q_soft_buffer, BUFFER_SIZE, PHASE_0, true);

#pragma omp parallel sections num_threads(2)
            {
#pragma omp section
                v1 = viterbi1.work(i_soft_buffer, BUFFER_SIZE, viterbi1_out);
#pragma omp section
                v2 = viterbi2.work(q_soft_buffer, BUFFER_SIZE, viterbi2_out);
            }
            vout = std::min(v1, v2);

            if (viterbi1.getState() == 0 || viterbi1.getState() == 0)
            {
                viterbiNoSyncRun++;

                if (viterbiNoSyncRun >= 10)
                {
                    if (shift == 0)
                        shift = 1;
                    else
                        shift = 0;
                }
            }

            // Perform differential decoding
            diff.work2(invert_branches ? viterbi1_out : viterbi2_out, invert_branches ? viterbi2_out : viterbi1_out, vout, diff_out);

            // Reconstruct into bytes and write to output file
            if (v1 > 0 && v2 > 0)
            {
                // Deframe / derand
                int frames = deframer.work(diff_out, vout * 2, frame_buffer);

                // Handle potential IQ swaps
                if (deframer.getState() == deframer.STATE_NOSYNC)
                {
                    noSyncRuns++;

                    if (noSyncRuns >= 10)
                    {
                        invert_branches = !invert_branches;
                        noSyncRuns = 0;
                    }
                }
                else
                {
                    noSyncRuns = 0;
                }

                for (int i = 0; i < frames; i++)
                {
                    uint8_t *cadu = &frame_buffer[i * 1024];

                    derand_ccsds(&cadu[4], 1024 - 4);

                    // RS Correction
                    rs.decode_interlaved(&cadu[4], true, 4, errors);

                    // Write it out
                    write_data((uint8_t *)cadu, 1024);
                }
            }
        }

        cleanup();
    }

    nlohmann::json FengyunMPTDecoderModule::getModuleStats()
    {
        auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
        v["deframer_lock"] = deframer.getState() == deframer.STATE_SYNCED;
        v["viterbi1_ber"] = viterbi1.ber();
        v["viterbi1_lock"] = viterbi1.getState();
        v["viterbi2_ber"] = viterbi2.ber();
        v["viterbi2_lock"] = viterbi2.getState();
        v["rs_avg"] = (errors[0] + errors[1] + errors[2] + errors[3]) / 4;
        std::string viterbi1_state = viterbi1.getState() == 0 ? "NOSYNC" : "SYNCED";
        std::string viterbi2_state = viterbi2.getState() == 0 ? "NOSYNC" : "SYNCED";
        std::string deframer_state = deframer.getState() == deframer.STATE_NOSYNC ? "NOSYNC" : (deframer.getState() == deframer.STATE_SYNCING ? "SYNCING" : "SYNCED");
        v["viterbi1_state"] = viterbi1_state;
        v["viterbi2_state"] = viterbi2_state;
        v["deframer_state"] = deframer_state;
        return v;
    }

    void FengyunMPTDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("FengYun MPT Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        float ber1 = viterbi1.ber();
        float ber2 = viterbi2.ber();

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

            ImGui::Button("Reed-Solomon", {200, 20});
            {
                ImGui::Text("RS    : ");
                for (int i = 0; i < 4; i++)
                {
                    ImGui::SameLine();

                    if (viterbi1.getState() == 0 || viterbi2.getState() == 0 || deframer.getState() == deframer.STATE_NOSYNC)
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

        ImGui::SameLine();

        ImGui::BeginGroup();
        {

            ImGui::Button("Viterbi 1", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (viterbi1.getState() == 0)
                    ImGui::TextColored(style::theme.red, "NOSYNC");
                else
                    ImGui::TextColored(style::theme.green, "SYNCED");

                ImGui::Text("BER   : ");
                ImGui::SameLine();
                ImGui::TextColored(viterbi1.getState() == 0 ? style::theme.red : style::theme.green, UITO_C_STR(ber1));

                std::memmove(&ber_history1[0], &ber_history1[1], (200 - 1) * sizeof(float));
                ber_history1[200 - 1] = ber1;

                widgets::ThemedPlotLines(style::theme.plot_bg.Value, "", ber_history1, IM_ARRAYSIZE(ber_history1), 0, "", 0.0f, 1.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
            }

            ImGui::Button("Viterbi 2", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (viterbi2.getState() == 0)
                    ImGui::TextColored(style::theme.red, "NOSYNC");
                else
                    ImGui::TextColored(style::theme.green, "SYNCED");

                ImGui::Text("BER   : ");
                ImGui::SameLine();
                ImGui::TextColored(viterbi2.getState() == 0 ? style::theme.red : style::theme.green, UITO_C_STR(ber2));

                std::memmove(&ber_history2[0], &ber_history2[1], (200 - 1) * sizeof(float));
                ber_history2[200 - 1] = ber2;

                widgets::ThemedPlotLines(style::theme.plot_bg.Value, "", ber_history2, IM_ARRAYSIZE(ber_history2), 0, "", 0.0f, 1.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
            }

            ImGui::Spacing();

            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (viterbi1.getState() == 0 || viterbi2.getState() == 0)
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
        }
        ImGui::EndGroup();

        drawProgressBar();

        ImGui::End();
    }

    std::string FengyunMPTDecoderModule::getID() { return "fengyun_mpt_decoder"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> FengyunMPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<FengyunMPTDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace fengyun3