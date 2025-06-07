#include "module_metop_dump_decoder.h"
#include "common/codings/correlator32.h"
#include "common/codings/randomization.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/codings/rotation.h"
#include "common/widgets/themed_widgets.h"
#include "logger.h"
#include <cstdint>

#define FRAME_SIZE 1024
#define ENCODED_FRAME_SIZE 1024 * 8

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace metop
{
    MetOpDumpDecoderModule::MetOpDumpDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        buffer = new uint8_t[ENCODED_FRAME_SIZE];
        fsfsm_file_ext = ".cadu";
    }

    MetOpDumpDecoderModule::~MetOpDumpDecoderModule() { delete[] buffer; }

    void MetOpDumpDecoderModule::process()
    {
        // Correlator
        Correlator32 correlator(QPSK, 0x1acffc1d);

        // Viterbi, rs, etc
        reedsolomon::ReedSolomon rs(reedsolomon::RS223);

        // Other buffers
        uint8_t frameBuffer[FRAME_SIZE];

        phase_t phase;
        bool swap;

        while (should_run())
        {
            // Read a buffer
            read_data(buffer, ENCODED_FRAME_SIZE);

            int pos = correlator.correlate((int8_t *)buffer, phase, swap, cor, ENCODED_FRAME_SIZE);

            locked = pos == 0; // Update locking state

            if (pos != 0 && pos < ENCODED_FRAME_SIZE) // Safety
            {
                std::memmove(buffer, &buffer[pos], pos);

                read_data(&buffer[ENCODED_FRAME_SIZE - pos], pos);
            }

            // Correct phase ambiguity
            rotate_soft((int8_t *)buffer, ENCODED_FRAME_SIZE, phase, swap);

            // Repack bits
            memset(frameBuffer, 0, FRAME_SIZE);
            for (int i = 0; i < ENCODED_FRAME_SIZE / 8; i++)
            {
                for (int y = 0; y < 8; y++)
                    frameBuffer[i] = frameBuffer[i] << 1 | (((int8_t *)buffer)[i * 8 + y] > 0);
                frameBuffer[i] ^= 0xFF;
            }

            // Derandomize that frame
            derand_ccsds(&frameBuffer[4], FRAME_SIZE - 4);

            // RS Correction
            rs.decode_interlaved(&frameBuffer[4], true, 4, errors);

            // Write it out
            uint8_t sync[4] = {0x1a, 0xcf, 0xfc, 0x1d};
            write_data(sync, 4);
            write_data(&frameBuffer[4], FRAME_SIZE - 4);
        }

        cleanup();
    }

    nlohmann::json MetOpDumpDecoderModule::getModuleStats()
    {
        auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
        std::string lock_state = locked ? "SYNCED" : "NOSYNC";
        v["lock_state"] = lock_state;
        return v;
    }

    void MetOpDumpDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("MetOp X-Band Dump Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        // float ber = viterbi.ber();

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
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)buffer)[i * 2 + 0] / 127.0) * 100 * ui_scale) % int(200 * ui_scale),
                                                      ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + (((int8_t *)buffer)[i * 2 + 1] / 127.0) * 100 * ui_scale) % int(200 * ui_scale)),
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
            ImGui::Button("Correlator", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("Corr  : ");
                ImGui::SameLine();
                ImGui::TextColored(locked ? style::theme.green : style::theme.orange, UITO_C_STR(cor));

                std::memmove(&cor_history[0], &cor_history[1], (200 - 1) * sizeof(float));
                cor_history[200 - 1] = cor;

                widgets::ThemedPlotLines(style::theme.plot_bg.Value, "", cor_history, IM_ARRAYSIZE(cor_history), 0, "", 40.0f, 64.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
            }

            ImGui::Spacing();

            ImGui::Button("Reed-Solomon", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("RS    : ");
                for (int i = 0; i < 4; i++)
                {
                    ImGui::SameLine();

                    if (errors[i] == -1)
                        ImGui::TextColored(style::theme.red, "%i ", i);
                    else if (errors[i] > 0)
                        ImGui::TextColored(style::theme.orange, "%i ", i);
                    else
                        ImGui::TextColored(style::theme.green, "%i ", i);
                }
            }
        }
        ImGui::EndGroup();

        drawProgressBar();

        ImGui::End();
    }

    std::string MetOpDumpDecoderModule::getID() { return "metop_dump_decoder"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> MetOpDumpDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<MetOpDumpDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace metop