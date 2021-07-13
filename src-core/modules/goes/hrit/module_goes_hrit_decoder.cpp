#include "module_goes_hrit_decoder.h"
#include "logger.h"
#include "common/sathelper/correlator.h"
#include "common/sathelper/packetfixer.h"
#include "common/sathelper/derandomizer.h"
#include "common/codings/differential/nrzm.h"
#include "imgui/imgui.h"
#include "common/codings/viterbi/viterbi27.h"
#include "common/codings/correlator.h"
#include "common/codings/reedsolomon/reedsolomon.h"

#define FRAME_SIZE 1024
#define ENCODED_FRAME_SIZE 1024 * 8 * 2

// Return filesize
size_t getFilesize(std::string filepath);

namespace goes
{
    namespace hrit
    {
        GOESHRITDecoderModule::GOESHRITDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                            viterbi(ENCODED_FRAME_SIZE / 2, viterbi::CCSDS_R2_K7_POLYS)
        {
            buffer = new uint8_t[ENCODED_FRAME_SIZE];
        }

        std::vector<ModuleDataType> GOESHRITDecoderModule::getInputTypes()
        {
            return {DATA_FILE, DATA_STREAM};
        }

        std::vector<ModuleDataType> GOESHRITDecoderModule::getOutputTypes()
        {
            return {DATA_FILE};
        }

        GOESHRITDecoderModule::~GOESHRITDecoderModule()
        {
            delete[] buffer;
        }

        void GOESHRITDecoderModule::process()
        {
            if (input_data_type == DATA_FILE)
                filesize = getFilesize(d_input_file);
            else
                filesize = 0;
            if (input_data_type == DATA_FILE)
                data_in = std::ifstream(d_input_file, std::ios::binary);
            if (output_data_type == DATA_FILE)
            {
                data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
                d_output_files.push_back(d_output_file_hint + ".cadu");
            }

            logger->info("Using input symbols " + d_input_file);
            logger->info("Decoding to " + d_output_file_hint + ".cadu");

            time_t lastTime = 0;

            // Correlator
            Correlator correlator(BPSK, 0xfc4ef4fd0cc2df89);

            // Viterbi, rs, etc
            sathelper::PacketFixer packetFixer;
            sathelper::Derandomizer derand;
            reedsolomon::ReedSolomon rs(reedsolomon::RS223);

            // Other buffers
            uint8_t frameBuffer[FRAME_SIZE];

            phase_t phase;
            bool swap;

            // Vectors are inverted
            diff::NRZMDiff diff;

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
                packetFixer.fixPacket(buffer, ENCODED_FRAME_SIZE, (sathelper::PhaseShift)phase, swap);

                // Viterbi
                viterbi.work((int8_t *)buffer, frameBuffer);

                // NRZ-M Decoding
                diff.decode(frameBuffer, FRAME_SIZE);

                // Derandomize that frame
                derand.work(&frameBuffer[4], FRAME_SIZE - 4);

                // RS Correction
                rs.decode_interlaved(&frameBuffer[4], true, 4, errors);

                // Write it out if it's not garbage
                if (errors[0] >= 0 && errors[1] >= 0 && errors[2] >= 0 && errors[3] >= 0)
                {
                    frameBuffer[0] = 0x1a;
                    frameBuffer[1] = 0xcf;
                    frameBuffer[2] = 0xfc;
                    frameBuffer[3] = 0x1d;

                    if (output_data_type == DATA_FILE)
                        data_out.write((char *)frameBuffer, FRAME_SIZE);
                    else
                        output_fifo->write(frameBuffer, FRAME_SIZE);
                }

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    std::string lock_state = locked ? "SYNCED" : "NOSYNC";
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Viterbi BER : " + std::to_string(viterbi.ber() * 100) + "%, Lock : " + lock_state);
                }
            }

            if (output_data_type == DATA_FILE)
                data_out.close();
            if (input_data_type == DATA_FILE)
                data_in.close();
        }

        void GOESHRITDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("GOES HRIT Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            float ber = viterbi.ber();

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
                        draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)buffer)[i] / 127.0) * 130 * ui_scale) % int(200 * ui_scale),
                                                          ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + rng.gasdev() * 14 * ui_scale) % int(200 * ui_scale)),
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

                ImGui::Spacing();

                ImGui::Button("Viterbi", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("BER   : ");
                    ImGui::SameLine();
                    ImGui::TextColored(ber < 0.22 ? IMCOLOR_SYNCED : IMCOLOR_NOSYNC, UITO_C_STR(ber));

                    std::memmove(&ber_history[0], &ber_history[1], (200 - 1) * sizeof(float));
                    ber_history[200 - 1] = ber;

                    ImGui::PlotLines("", ber_history, IM_ARRAYSIZE(ber_history), 0, "", 0.0f, 1.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
                }

                ImGui::Spacing();

                ImGui::Button("Reed-Solomon", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("RS    : ");
                    for (int i = 0; i < 4; i++)
                    {
                        ImGui::SameLine();

                        if (errors[i] == -1)
                            ImGui::TextColored(IMCOLOR_NOSYNC, "%i ", i);
                        else if (errors[i] > 0)
                            ImGui::TextColored(IMCOLOR_SYNCING, "%i ", i);
                        else
                            ImGui::TextColored(IMCOLOR_SYNCED, "%i ", i);
                    }
                }
            }
            ImGui::EndGroup();

            if (!streamingInput)
                ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string GOESHRITDecoderModule::getID()
        {
            return "goes_hrit_decoder";
        }

        std::vector<std::string> GOESHRITDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> GOESHRITDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<GOESHRITDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace meteor
}