#include "module_goes_mdl_decoder.h"
#include "logger.h"
#include "common/codings/rotation.h"
#include "imgui/imgui.h"
#include "common/codings/correlator32.h"

#define FRAME_SIZE 464
#define ENCODED_FRAME_SIZE 464 * 8

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace goes
{
    namespace mdl
    {
        GOESMDLDecoderModule::GOESMDLDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
            buffer = new uint8_t[ENCODED_FRAME_SIZE];
        }

        std::vector<ModuleDataType> GOESMDLDecoderModule::getInputTypes()
        {
            return {DATA_FILE, DATA_STREAM};
        }

        std::vector<ModuleDataType> GOESMDLDecoderModule::getOutputTypes()
        {
            return {DATA_FILE};
        }

        GOESMDLDecoderModule::~GOESMDLDecoderModule()
        {
            delete[] buffer;
        }

        void GOESMDLDecoderModule::process()
        {
            if (input_data_type == DATA_FILE)
                filesize = getFilesize(d_input_file);
            else
                filesize = 0;
            if (input_data_type == DATA_FILE)
                data_in = std::ifstream(d_input_file, std::ios::binary);
            data_out = std::ofstream(d_output_file_hint + ".frm", std::ios::binary);
            d_output_files.push_back(d_output_file_hint + ".frm");

            logger->info("Using input symbols " + d_input_file);
            logger->info("Decoding to " + d_output_file_hint + ".frm");

            time_t lastTime = 0;

            // Correlator
            Correlator32 correlator(QPSK, 0b00010111110101111001100100000 << 3);

            // Other buffers
            uint8_t frameBuffer[FRAME_SIZE];

            phase_t phase;
            bool swap;

            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                // Read a buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)buffer, ENCODED_FRAME_SIZE);
                else
                    input_fifo->read((uint8_t *)buffer, ENCODED_FRAME_SIZE);

                int pos = correlator.correlate((int8_t *)buffer, phase, swap, cor, ENCODED_FRAME_SIZE);

                locked = pos == 0; // Update locking state

                if (pos != 0 && pos < ENCODED_FRAME_SIZE) // Safety
                {
                    std::memmove(buffer, &buffer[pos], pos);

                    if (input_data_type == DATA_FILE)
                        data_in.read((char *)&buffer[ENCODED_FRAME_SIZE - pos], pos);
                    else
                        input_fifo->read((uint8_t *)&buffer[ENCODED_FRAME_SIZE - pos], pos);
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
                data_out.write((char *)&frameBuffer[0], FRAME_SIZE);

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    std::string lock_state = locked ? "SYNCED" : "NOSYNC";
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%, Lock : " + lock_state);
                }
            }

            data_out.close();
            if (input_data_type == DATA_FILE)
                data_in.close();
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
                    draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                             ImVec2(ImGui::GetCursorScreenPos().x + 200 * ui_scale, ImGui::GetCursorScreenPos().y + 200 * ui_scale),
                                             ImColor::HSV(0, 0, 0));

                    for (int i = 0; i < 2048; i++)
                    {
                        draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)buffer)[i * 2 + 0] / 127.0) * 100 * ui_scale) % int(200 * ui_scale),
                                                          ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + (((int8_t *)buffer)[i * 2 + 1] / 127.0) * 100 * ui_scale) % int(200 * ui_scale)),
                                                   2 * ui_scale,
                                                   ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
                    }

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
                    ImGui::TextColored(locked ? IMCOLOR_SYNCED : IMCOLOR_SYNCING, UITO_C_STR(cor));

                    std::memmove(&cor_history[0], &cor_history[1], (200 - 1) * sizeof(float));
                    cor_history[200 - 1] = cor;

                    ImGui::PlotLines("", cor_history, IM_ARRAYSIZE(cor_history), 0, "", 40.0f, 64.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
                }
            }
            ImGui::EndGroup();

            if (!streamingInput)
                ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string GOESMDLDecoderModule::getID()
        {
            return "goes_mdl_decoder";
        }

        std::vector<std::string> GOESMDLDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> GOESMDLDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GOESMDLDecoderModule>(input_file, output_file_hint, parameters);
        }
    }
} // namespace meteor