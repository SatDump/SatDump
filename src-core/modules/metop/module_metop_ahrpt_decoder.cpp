#include "module_metop_ahrpt_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace metop
{
    MetOpAHRPTDecoderModule::MetOpAHRPTDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                            d_viterbi_outsync_after(std::stoi(parameters["viterbi_outsync_after"])),
                                                                                                                                                            d_viterbi_ber_threasold(std::stof(parameters["viterbi_ber_thresold"])),
                                                                                                                                                            d_soft_symbols(std::stoi(parameters["soft_symbols"])),
                                                                                                                                                            sw(0),
                                                                                                                                                            viterbi(true, d_viterbi_ber_threasold, 1, d_viterbi_outsync_after, 50, BUFFER_SIZE)
    {
        viterbi_out = new uint8_t[BUFFER_SIZE];
        sym_buffer = new std::complex<float>[BUFFER_SIZE];
        soft_buffer = new int8_t[BUFFER_SIZE * 2];
    }

    MetOpAHRPTDecoderModule::~MetOpAHRPTDecoderModule()
    {
        delete[] viterbi_out;
        delete[] sym_buffer;
        delete[] soft_buffer;
    }

    void MetOpAHRPTDecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".cadu");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        while (!data_in.eof())
        {
            // Read a buffer
            if (d_soft_symbols)
            {
                data_in.read((char *)soft_buffer, BUFFER_SIZE * 2);

                // Convert to hard symbols from soft symbols. We may want to work with soft only later?
                for (int i = 0; i < BUFFER_SIZE; i++)
                {
                    using namespace std::complex_literals;
                    sym_buffer[i] = ((float)soft_buffer[i * 2 + 1] / 127.0f) + ((float)soft_buffer[i * 2] / 127.0f) * 1if;
                }
            }
            else
            {
                data_in.read((char *)sym_buffer, sizeof(std::complex<float>) * BUFFER_SIZE);
            }

            // Push into Viterbi
            int num_samp = viterbi.work(sym_buffer, BUFFER_SIZE, viterbi_out);

            // Reconstruct into bytes and write to output file
            if (num_samp > 0)
            {
                // Deframe / derand
                std::vector<std::array<uint8_t, CADU_SIZE>> frames = deframer.work(viterbi_out, num_samp);

                if (frames.size() > 0)
                {
                    for (std::array<uint8_t, CADU_SIZE> cadu : frames)
                    {
                        // RS Decoding
                        for (int i = 0; i < 4; i++)
                        {
                            reedSolomon.deinterleave(&cadu[4], rsWorkBuffer, i, 4);
                            errors[i] = reedSolomon.decode_ccsds(rsWorkBuffer);
                            reedSolomon.interleave(rsWorkBuffer, &cadu[4], i, 4);
                        }

                        // Write it out
                        //data_out_total += CADU_SIZE;
                        data_out.write((char *)&cadu, CADU_SIZE);
                    }
                }
            }

            progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string viterbi_state = viterbi.getState() == 0 ? "NOSYNC" : "SYNCED";
                std::string deframer_state = deframer.getState() == 0 ? "NOSYNC" : (deframer.getState() == 2 || deframer.getState() == 6 ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Viterbi : " + viterbi_state + " BER : " + std::to_string(viterbi.ber()) + ", Deframer : " + deframer_state);
            }
        }

        data_out.close();
        data_in.close();
    }

    const ImColor colorNosync = ImColor::HSV(0 / 360.0, 1, 1, 1.0);
    const ImColor colorSyncing = ImColor::HSV(39.0 / 360.0, 0.93, 1, 1.0);
    const ImColor colorSynced = ImColor::HSV(113.0 / 360.0, 1, 1, 1.0);

    void MetOpAHRPTDecoderModule::drawUI()
    {
        ImGui::Begin("MetOp AHRPT Decoder", NULL, NOWINDOW_FLAGS);

        float ber = viterbi.ber();

        ImGui::BeginGroup();
        {
            // Constellation
            {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                         ImVec2(ImGui::GetCursorScreenPos().x + 200, ImGui::GetCursorScreenPos().y + 200),
                                         ImColor::HSV(0, 0, 0));

                for (int i = 0; i < 2048; i++)
                {
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 + sym_buffer[i].real() * 100) % 200,
                                                      ImGui::GetCursorScreenPos().y + (int)(100 + sym_buffer[i].imag() * 100) % 200),
                                               2,
                                               ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
                }

                ImGui::Dummy(ImVec2(200 + 3, 200 + 3));
            }
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Viterbi", {200, 20});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (viterbi.getState() == 0)
                    ImGui::TextColored(colorNosync, "NOSYNC");
                else
                    ImGui::TextColored(colorSynced, "SYNCED");

                ImGui::Text("BER   : ");
                ImGui::SameLine();
                ImGui::TextColored(viterbi.getState() == 0 ? colorNosync : colorSynced, std::to_string(ber).c_str());

                std::memmove(&ber_history[0], &ber_history[1], (200 - 1) * sizeof(float));
                ber_history[200 - 1] = ber;

                ImGui::PlotLines("", ber_history, IM_ARRAYSIZE(ber_history), 0, "", 0.0f, 1.0f, ImVec2(200, 50));
            }

            ImGui::Spacing();

            ImGui::Button("Deframer", {200, 20});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (deframer.getState() == 0)
                    ImGui::TextColored(colorNosync, "NOSYNC");
                else if (deframer.getState() == 2 || deframer.getState() == 6)
                    ImGui::TextColored(colorSyncing, "SYNCING");
                else
                    ImGui::TextColored(colorSynced, "SYNCED");
            }

            ImGui::Spacing();

            ImGui::Button("Reed-Solomon", {200, 20});
            {
                ImGui::Text("RS    : ");
                for (int i = 0; i < 4; i++)
                {
                    ImGui::SameLine();

                    if (errors[i] == -1)
                        ImGui::TextColored(colorNosync, "%i ", i);
                    else if (errors[i] > 0)
                        ImGui::TextColored(colorSyncing, "%i ", i);
                    else
                        ImGui::TextColored(colorSynced, "%i ", i);
                }
            }
        }
        ImGui::EndGroup();

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20));

        ImGui::End();
    }

    std::string MetOpAHRPTDecoderModule::getID()
    {
        return "metop_ahrpt_decoder";
    }

    std::vector<std::string> MetOpAHRPTDecoderModule::getParameters()
    {
        return {"viterbi_outsync_after", "viterbi_ber_thresold", "soft_symbols"};
    }

    std::shared_ptr<ProcessingModule> MetOpAHRPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<MetOpAHRPTDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace metop