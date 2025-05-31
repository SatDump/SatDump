#include "module_goes_mdl_decoder.h"
#include "common/codings/correlator32.h"
#include "common/codings/rotation.h"
#include "common/widgets/themed_widgets.h"
#include "logger.h"
#include <cstdint>

#define FRAME_SIZE 464
#define ENCODED_FRAME_SIZE 464 * 8

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace goes
{
    namespace mdl
    {
        GOESMDLDecoderModule::GOESMDLDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            buffer = new uint8_t[ENCODED_FRAME_SIZE];
            fsfsm_file_ext = ".frm";
        }

        GOESMDLDecoderModule::~GOESMDLDecoderModule() { delete[] buffer; }

        void GOESMDLDecoderModule::process()
        {
            // Correlator
            Correlator32 correlator(QPSK, 0b00010111110101111001100100000 << 3);

            // Other buffers
            uint8_t frameBuffer[FRAME_SIZE];

            phase_t phase;
            bool swap;

            while (should_run())
            {
                // Read a buffer
                read_data((uint8_t *)buffer, ENCODED_FRAME_SIZE);

                int pos = correlator.correlate((int8_t *)buffer, phase, swap, cor, ENCODED_FRAME_SIZE);

                locked = pos == 0; // Update locking state

                if (pos != 0 && pos < ENCODED_FRAME_SIZE) // Safety
                {
                    std::memmove(buffer, &buffer[pos], pos);

                    read_data((uint8_t *)&buffer[ENCODED_FRAME_SIZE - pos], pos);
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

                // Write it out
                // data_out.put(0xFA);
                // data_out.put(0xF3);
                // data_out.put(0x20);
                write_data((uint8_t *)&frameBuffer[0], FRAME_SIZE);
            }

            cleanup();
        }

        nlohmann::json GOESMDLDecoderModule::getModuleStats()
        {
            auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
            std::string lock_state = locked ? "SYNCED" : "NOSYNC";
            v["lock_state"] = lock_state;
            return v;
        }

        void GOESMDLDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("GOES MDL Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

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
            }
            ImGui::EndGroup();

            drawProgressBar();

            ImGui::End();
        }

        std::string GOESMDLDecoderModule::getID() { return "goes_mdl_decoder"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> GOESMDLDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GOESMDLDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace mdl
} // namespace goes