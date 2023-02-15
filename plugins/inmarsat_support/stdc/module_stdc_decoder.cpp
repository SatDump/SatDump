#include "module_stdc_decoder.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"

namespace inmarsat
{
    namespace stdc
    {
        STDCDecoderModule::STDCDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                viterbi(ENCODED_FRAME_SIZE_NOSYNC / 2, {109, 79})
        {
            buffer = new int8_t[ENCODED_FRAME_SIZE];
            buffer_shifter = new int8_t[ENCODED_FRAME_SIZE];
            buffer_synchronized = new int8_t[ENCODED_FRAME_SIZE];
            buffer_depermuted = new int8_t[ENCODED_FRAME_SIZE];
            buffer_vitdecoded = new uint8_t[ENCODED_FRAME_SIZE];
        }

        std::vector<ModuleDataType> STDCDecoderModule::getInputTypes()
        {
            return {DATA_FILE, DATA_STREAM};
        }

        std::vector<ModuleDataType> STDCDecoderModule::getOutputTypes()
        {
            return {DATA_FILE, DATA_STREAM};
        }

        STDCDecoderModule::~STDCDecoderModule()
        {
            delete[] buffer;
            delete[] buffer_shifter;
            delete[] buffer_synchronized;
            delete[] buffer_depermuted;
            delete[] buffer_vitdecoded;
        }

        void STDCDecoderModule::process()
        {
            if (input_data_type == DATA_FILE)
                filesize = getFilesize(d_input_file);
            else
                filesize = 0;
            if (input_data_type == DATA_FILE)
                data_in = std::ifstream(d_input_file, std::ios::binary);
            if (output_data_type == DATA_FILE)
            {
                data_out = std::ofstream(d_output_file_hint + ".frm", std::ios::binary);
                d_output_files.push_back(d_output_file_hint + ".frm");
            }

            logger->info("Using input symbols " + d_input_file);
            logger->info("Decoding to " + d_output_file_hint + ".frm");

            time_t lastTime = 0;
            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                // Read a buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)buffer, ENCODED_FRAME_SIZE);
                else
                    input_fifo->read((uint8_t *)buffer, ENCODED_FRAME_SIZE);

                gotFrame = false;

                uint16_t frm_num = 0;

                for (int i = 0; i < ENCODED_FRAME_SIZE; i++)
                {
                    memmove(&buffer_shifter[0], &buffer_shifter[1], ENCODED_FRAME_SIZE - 1);
                    buffer_shifter[ENCODED_FRAME_SIZE - 1] = buffer[i];

                    bool inverted = false;
                    int best_match = compute_frame_match(buffer_shifter, inverted);
                    if (best_match > 120)
                    {
                        for (int b = 0; b < ENCODED_FRAME_SIZE; b++)
                        {
                            if (inverted)
                                buffer_synchronized[b] = ~buffer_shifter[b];
                            else
                                buffer_synchronized[b] = buffer_shifter[b];
                        }

                        cor = best_match;

                        depermute(buffer_synchronized, buffer_depermuted);
                        deinterleave(buffer_depermuted, buffer_synchronized);
                        viterbi.work(buffer_synchronized, buffer_vitdecoded);
                        descramble(buffer_vitdecoded);

                        frm_num = buffer_vitdecoded[2] << 8 | buffer_vitdecoded[3];
                        logger->trace("Got STD-C Frame Corr {:d} Inv {:d} Ber {:f} No {:d}", best_match, (int)inverted, viterbi.ber(), frm_num);

                        if (output_data_type == DATA_FILE)
                            data_out.write((char *)buffer_vitdecoded, ENCODED_FRAME_SIZE_NOSYNC / 16);
                        else
                            output_fifo->write((uint8_t *)buffer_vitdecoded, ENCODED_FRAME_SIZE_NOSYNC / 16);

                        gotFrame = true;
                    }
                }

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                // Update module stats
                module_stats["deframer_lock"] = gotFrame;
                module_stats["viterbi_ber"] = viterbi.ber();
                module_stats["last_count"] = frm_num;

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    std::string lock_state = gotFrame ? "SYNCED" : "NOSYNC";
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Viterbi BER : " + std::to_string(viterbi.ber() * 100) + "%, Lock : " + lock_state);
                }
            }

            if (input_data_type == DATA_FILE)
                data_in.close();
            if (output_data_type == DATA_FILE)
                data_out.close();
        }

        void STDCDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Inmarsat STD-C Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            float ber = viterbi.ber();

            ImGui::BeginGroup();
            {
                ImGui::Button("Correlator", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("Corr  : ");
                    ImGui::SameLine();
                    ImGui::TextColored(gotFrame ? IMCOLOR_SYNCED : IMCOLOR_SYNCING, UITO_C_STR(cor));

                    std::memmove(&cor_history[0], &cor_history[1], (200 - 1) * sizeof(float));
                    cor_history[200 - 1] = cor;

                    ImGui::PlotLines("", cor_history, IM_ARRAYSIZE(cor_history), 0, "", 60.0f, 128.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
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
            }
            ImGui::EndGroup();

            if (input_data_type == DATA_FILE)
                ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string STDCDecoderModule::getID()
        {
            return "inmarsat_stdc_decoder";
        }

        std::vector<std::string> STDCDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> STDCDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<STDCDecoderModule>(input_file, output_file_hint, parameters);
        }
    }
}