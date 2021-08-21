#include "module_npp_hrd_decoder.h"
#include "logger.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/codings/differential/nrzm.h"
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace npp
{
    NPPHRDDecoderModule::NPPHRDDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
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

    NPPHRDDecoderModule::~NPPHRDDecoderModule()
    {
        delete[] viterbi_out;
        delete[] sym_buffer;
        delete[] soft_buffer;
    }

    void NPPHRDDecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".cadu");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        diff::NRZMDiff diff;

        reedsolomon::ReedSolomon rs(reedsolomon::RS223);

        int vout;

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
                    sym_buffer[i] = ((float)soft_buffer[i * 2 + 0] / 127.0f) + ((float)soft_buffer[i * 2 + 1] / 127.0f) * 1if;
                }
            }
            else
            {
                data_in.read((char *)sym_buffer, sizeof(std::complex<float>) * BUFFER_SIZE);
            }

            // Perform Viterbi decoding
            vout = viterbi.work(sym_buffer, BUFFER_SIZE, viterbi_out);

            // Perform differential decoding
            diff.decode(viterbi_out, vout);

            if (vout > 0)
            {
                // Deframe (and derand)
                std::vector<std::array<uint8_t, ccsds::ccsds_1_0_1024::CADU_SIZE>> frames = deframer.work(viterbi_out, vout);

                // if we found frame, write them
                if (frames.size() > 0)
                {
                    for (std::array<uint8_t, ccsds::ccsds_1_0_1024::CADU_SIZE> cadu : frames)
                    {
                        // RS Correction
                        rs.decode_interlaved(&cadu[4], true, 4, errors);

                        // Write it out
                        //data_out_total += ccsds::ccsds_1_0_1024::CADU_SIZE;
                        data_out.write((char *)&cadu, ccsds::ccsds_1_0_1024::CADU_SIZE);
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

    void NPPHRDDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("NPP HRD Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);
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
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + sym_buffer[i].real() * 100 * ui_scale) % int(200 * ui_scale),
                                                      ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + sym_buffer[i].imag() * 100 * ui_scale) % int(200 * ui_scale)),
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
            ImGui::Button("Viterbi", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (viterbi.getState() == 0)
                    ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                else
                    ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");

                ImGui::Text("BER   : ");
                ImGui::SameLine();
                ImGui::TextColored(viterbi.getState() == 0 ? IMCOLOR_NOSYNC : IMCOLOR_SYNCED, UITO_C_STR(ber));

                std::memmove(&ber_history[0], &ber_history[1], (200 - 1) * sizeof(float));
                ber_history[200 - 1] = ber;

                ImGui::PlotLines("", ber_history, IM_ARRAYSIZE(ber_history), 0, "", 0.0f, 1.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
            }

            ImGui::Spacing();

            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (deframer.getState() == 0)
                    ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                else if (deframer.getState() == 2 || deframer.getState() == 6)
                    ImGui::TextColored(IMCOLOR_SYNCING, "SYNCING");
                else
                    ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");
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

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string NPPHRDDecoderModule::getID()
    {
        return "npp_hrd_decoder";
    }

    std::vector<std::string> NPPHRDDecoderModule::getParameters()
    {
        return {"viterbi_outsync_after", "viterbi_ber_thresold", "soft_symbols"};
    }

    std::shared_ptr<ProcessingModule> NPPHRDDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<NPPHRDDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace npp