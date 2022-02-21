#include "module_new_fengyun_mpt_decoder.h"
#include "logger.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/codings/rotation.h"
#include "diff.h"
#include "common/utils.h"
#include "libs/ctpl/ctpl_stl.h"
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192 * 4

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun3
{
    NewFengyunMPTDecoderModule::NewFengyunMPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                              d_viterbi_outsync_after(parameters["viterbi_outsync_after"].get<int>()),
                                                                                                                                              d_viterbi_ber_threasold(parameters["viterbi_ber_thresold"].get<float>()),
                                                                                                                                              d_soft_symbols(parameters["soft_symbols"].get<bool>()),
                                                                                                                                              viterbi1(d_viterbi_ber_threasold, d_viterbi_outsync_after, BUFFER_SIZE / 2),
                                                                                                                                              viterbi2(d_viterbi_ber_threasold, d_viterbi_outsync_after, BUFFER_SIZE / 2)
    {
        viterbi_out = new uint8_t[BUFFER_SIZE];
        soft_buffer = new int8_t[BUFFER_SIZE * 2];
        viterbi1_out = new uint8_t[BUFFER_SIZE];
        viterbi2_out = new uint8_t[BUFFER_SIZE];
        iSamples = new int8_t[BUFFER_SIZE];
        qSamples = new int8_t[BUFFER_SIZE];
        diff_in = new uint8_t[BUFFER_SIZE];
        diff_out = new uint8_t[BUFFER_SIZE];
    }

    NewFengyunMPTDecoderModule::~NewFengyunMPTDecoderModule()
    {
        delete[] viterbi_out;
        delete[] soft_buffer;
        delete[] viterbi1_out;
        delete[] viterbi2_out;
        delete[] iSamples;
        delete[] qSamples;
        delete[] diff_in;
        delete[] diff_out;
    }

    void NewFengyunMPTDecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".cadu");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        // Our 2 Viterbi decoders and differential decoder
        FengyunDiff diff;

        reedsolomon::ReedSolomon rs(reedsolomon::RS223);

        uint8_t frameBuffer[BUFFER_SIZE];
        int inFrameBuffer = 0;

        // Util values
        int shift = 0;
        bool iq_invert = false;
        int noSyncRuns = 0, viterbiNoSyncRun = 0;

        while (!data_in.eof())
        {
            data_in.read((char *)soft_buffer, BUFFER_SIZE);

            rotate_soft(soft_buffer, BUFFER_SIZE, PHASE_0, iq_invert);

            // Deinterleave I & Q for the 2 Viterbis
            for (int i = 0; i < BUFFER_SIZE / 2; i++)
            {
                iSamples[i] = soft_buffer[(i + shift) * 2 + 0];
                qSamples[i] = ~soft_buffer[(i + shift) * 2 + 1]; // Second is inverted
            }

            // Run Viterbi!
            v1 = viterbi1.work((uint8_t *)qSamples, BUFFER_SIZE / 2, viterbi1_out);
            v2 = viterbi2.work((uint8_t *)iSamples, BUFFER_SIZE / 2, viterbi2_out);

            diffin = 0;

            // Interleave and pack output into 2 bits chunks
            if (v1 > 0 && v2 > 0)
            {
                for (int y = 0; y < std::min<int>(v1, v2); y++)
                {
                    for (int i = 7; i >= 0; i--)
                    {
                        diff_in[diffin++] = getBit<uint8_t>(viterbi2_out[y], i) << 1 | getBit<uint8_t>(viterbi1_out[y], i);
                    }
                }

                viterbiNoSyncRun = 0;
            }

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
            diff.work(diff_in, diffin, diff_out);

            // Reconstruct into bytes and write to output file
            if (diffin > 0)
            {

                inFrameBuffer = 0;
                // Reconstruct into bytes and write to output file
                for (int i = 0; i < diffin / 4; i++)
                {
                    frameBuffer[inFrameBuffer++] = diff_out[i * 4] << 6 | diff_out[i * 4 + 1] << 4 | diff_out[i * 4 + 2] << 2 | diff_out[i * 4 + 3];
                }

                // Deframe / derand
                std::vector<std::array<uint8_t, ccsds::ccsds_1_0_1024::CADU_SIZE>> frames = deframer.work(frameBuffer, inFrameBuffer);

                if (frames.size() > 0)
                {
                    for (std::array<uint8_t, ccsds::ccsds_1_0_1024::CADU_SIZE> cadu : frames)
                    {
                        // RS Decoding
                        rs.decode_interlaved(&cadu[4], true, 4, errors);

                        // Write it out
                        data_out.write((char *)&cadu, ccsds::ccsds_1_0_1024::CADU_SIZE);
                    }
                }

                if (deframer.getState() == 0)
                {
                    noSyncRuns++;

                    if (noSyncRuns >= 10)
                    {
                        iq_invert = !iq_invert;
                        noSyncRuns = 0;
                    }
                }
                else
                {
                    noSyncRuns = 0;
                    viterbiNoSyncRun = 0;
                }
            }

            progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string viterbi1_state = viterbi1.getState() == 0 ? "NOSYNC" : "SYNCED";
                std::string viterbi2_state = viterbi2.getState() == 0 ? "NOSYNC" : "SYNCED";
                std::string deframer_state = deframer.getState() == 0 ? "NOSYNC" : (deframer.getState() == 2 || deframer.getState() == 6 ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Viterbi 1 : " + viterbi1_state + " BER : " + std::to_string(viterbi1.ber()) + ", Viterbi 2 : " + viterbi2_state + " BER : " + std::to_string(viterbi2.ber()) + ", Deframer : " + deframer_state);
            }
        }

        data_out.close();
        data_in.close();
    }

    void NewFengyunMPTDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("New FengYun MPT Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

        float ber1 = viterbi1.ber();
        float ber2 = viterbi2.ber();

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
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)soft_buffer)[i * 2 + 0] / 127.0) * 100 * ui_scale) % int(200 * ui_scale),
                                                      ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + (((int8_t *)soft_buffer)[i * 2 + 1] / 127.0) * 100 * ui_scale) % int(200 * ui_scale)),
                                               2 * ui_scale,
                                               ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
                }

                ImGui::Dummy(ImVec2(200 * ui_scale + 3, 200 * ui_scale + 3));
            }

            ImGui::Button("Reed-Solomon", {200, 20});
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

        ImGui::SameLine();

        ImGui::BeginGroup();
        {

            ImGui::Button("Viterbi 1", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (viterbi1.getState() == 0)
                    ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                else
                    ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");

                ImGui::Text("BER   : ");
                ImGui::SameLine();
                ImGui::TextColored(viterbi1.getState() == 0 ? IMCOLOR_NOSYNC : IMCOLOR_SYNCED, UITO_C_STR(ber1));

                std::memmove(&ber_history1[0], &ber_history1[1], (200 - 1) * sizeof(float));
                ber_history1[200 - 1] = ber1;

                ImGui::PlotLines("", ber_history1, IM_ARRAYSIZE(ber_history1), 0, "", 0.0f, 1.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
            }

            ImGui::Button("Viterbi 2", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (viterbi2.getState() == 0)
                    ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                else
                    ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");

                ImGui::Text("BER   : ");
                ImGui::SameLine();
                ImGui::TextColored(viterbi2.getState() == 0 ? IMCOLOR_NOSYNC : IMCOLOR_SYNCED, UITO_C_STR(ber2));

                std::memmove(&ber_history2[0], &ber_history2[1], (200 - 1) * sizeof(float));
                ber_history2[200 - 1] = ber2;

                ImGui::PlotLines("", ber_history2, IM_ARRAYSIZE(ber_history2), 0, "", 0.0f, 1.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
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
        }
        ImGui::EndGroup();

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string NewFengyunMPTDecoderModule::getID()
    {
        return "new_fengyun_mpt_decoder";
    }

    std::vector<std::string> NewFengyunMPTDecoderModule::getParameters()
    {
        return {"viterbi_outsync_after", "viterbi_ber_thresold", "soft_symbols", "invert_second_viterbi"};
    }

    std::shared_ptr<ProcessingModule> NewFengyunMPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<NewFengyunMPTDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace fengyun