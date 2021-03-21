#include "module_fengyun_mpt_decoder.h"
#include "logger.h"
#include "modules/common/sathelper/reedsolomon_233.h"
#include "modules/common/sathelper/packetfixer.h"
#include "diff.h"
#include "modules/metop/instruments/iasi/utils.h"
#include "modules/common/ctpl/ctpl_stl.h"
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun
{
    FengyunMPTDecoderModule::FengyunMPTDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                            d_viterbi_outsync_after(std::stoi(parameters["viterbi_outsync_after"])),
                                                                                                                                                            d_viterbi_ber_threasold(std::stof(parameters["viterbi_ber_thresold"])),
                                                                                                                                                            d_soft_symbols(std::stoi(parameters["soft_symbols"])),
                                                                                                                                                            viterbi1(true, d_viterbi_ber_threasold, 1, d_viterbi_outsync_after, 50, BUFFER_SIZE),
                                                                                                                                                            viterbi2(true, d_viterbi_ber_threasold, 1, d_viterbi_outsync_after, 50, BUFFER_SIZE)
    {
        viterbi_out = new uint8_t[BUFFER_SIZE];
        sym_buffer = new std::complex<float>[BUFFER_SIZE];
        soft_buffer = new int8_t[BUFFER_SIZE * 2];
        viterbi1_out = new uint8_t[BUFFER_SIZE];
        viterbi2_out = new uint8_t[BUFFER_SIZE];
        iSamples = new std::complex<float>[BUFFER_SIZE];
        qSamples = new std::complex<float>[BUFFER_SIZE];
        diff_in = new uint8_t[BUFFER_SIZE];
        diff_out = new uint8_t[BUFFER_SIZE];
    }

    FengyunMPTDecoderModule::~FengyunMPTDecoderModule()
    {
        delete[] viterbi_out;
        delete[] sym_buffer;
        delete[] soft_buffer;
        delete[] viterbi1_out;
        delete[] viterbi2_out;
        delete[] iSamples;
        delete[] qSamples;
        delete[] diff_in;
        delete[] diff_out;
    }

    void FengyunMPTDecoderModule::process()
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

        sathelper::ReedSolomon reedSolomon;

        uint8_t frameBuffer[BUFFER_SIZE];
        int inFrameBuffer = 0;

        // Multithreading stuff
        ctpl::thread_pool viterbi_pool(2);

        // Tests
        sathelper::PhaseShift shift = sathelper::DEG_0;
        sathelper::PacketFixer shifter;
        bool iq_invert = false;
        int noSyncRuns = 0, viterbiNoSyncRun = 0;

        while (!data_in.eof())
        {

            // Read a buffer
            if (d_soft_symbols)
            {
                data_in.read((char *)soft_buffer, BUFFER_SIZE * 2);

                shifter.fixPacket((uint8_t *)soft_buffer, BUFFER_SIZE * 2, shift, iq_invert);

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

            inI = 0;
            inQ = 0;

            // Deinterleave I & Q for the 2 Viterbis
            for (int i = 0; i < BUFFER_SIZE / 2; i++)
            {
                using namespace std::complex_literals;
                std::complex<float> iS = sym_buffer[i * 2 + shift].imag() + sym_buffer[i * 2 + 1 + shift].imag() * 1if;
                std::complex<float> qS = sym_buffer[i * 2 + shift].real() + sym_buffer[i * 2 + 1 + shift].real() * 1if;
                iSamples[inI++] = iS;
                qSamples[inQ++] = -qS; //FY3C
            }

            // Run Viterbi!
            v1_fut = viterbi_pool.push([&](int) { v1 = viterbi1.work(qSamples, inQ, viterbi1_out); });
            v2_fut = viterbi_pool.push([&](int) { v2 = viterbi2.work(iSamples, inI, viterbi2_out); });
            v1_fut.get();
            v2_fut.get();

            diffin = 0;

            // Interleave and pack output into 2 bits chunks
            if (v1 > 0 || v2 > 0)
            {
                if (v1 == v2)
                {
                    uint8_t bit1, bit2;
                    for (int y = 0; y < v1; y++)
                    {
                        for (int i = 7; i >= 0; i--)
                        {
                            bit1 = getBit<uint8_t>(viterbi1_out[y], i);
                            bit2 = getBit<uint8_t>(viterbi2_out[y], i);
                            diff_in[diffin++] = bit2 << 1 | bit1;
                        }
                    }
                }
                viterbiNoSyncRun = 0;
            }

            if (viterbi1.getState() == 0 || viterbi1.getState() == 0)
            {
                viterbiNoSyncRun++;

                if (viterbiNoSyncRun == 10)
                {
                    if (shift == sathelper::DEG_0)
                        shift = sathelper::DEG_90;
                    else
                        shift = sathelper::DEG_0;
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
                        for (int i = 0; i < 4; i++)
                        {
                            reedSolomon.deinterleave(&cadu[4], rsWorkBuffer, i, 4);
                            errors[i] = reedSolomon.decode_ccsds(rsWorkBuffer);
                            reedSolomon.interleave(rsWorkBuffer, &cadu[4], i, 4);
                        }

                        // Write it out
                        //data_out_total += ccsds::ccsds_1_0_1024::CADU_SIZE;
                        data_out.write((char *)&cadu, ccsds::ccsds_1_0_1024::CADU_SIZE);
                    }
                }

                if (deframer.getState() == 0)
                    noSyncRuns++;
                else
                {
                    noSyncRuns = 0;
                    viterbiNoSyncRun = 0;
                }
                if (noSyncRuns == 10)
                {
                    iq_invert = !iq_invert;
                    noSyncRuns = 0;
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

    const ImColor colorNosync = ImColor::HSV(0 / 360.0, 1, 1, 1.0);
    const ImColor colorSyncing = ImColor::HSV(39.0 / 360.0, 0.93, 1, 1.0);
    const ImColor colorSynced = ImColor::HSV(113.0 / 360.0, 1, 1, 1.0);

    void FengyunMPTDecoderModule::drawUI()
    {
        ImGui::Begin("FengYun MPT Decoder", NULL, NOWINDOW_FLAGS);

        float ber1 = viterbi1.ber();
        float ber2 = viterbi2.ber();

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

        ImGui::SameLine();

        ImGui::BeginGroup();
        {

            ImGui::Button("Viterbi 1", {200, 20});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (viterbi1.getState() == 0)
                    ImGui::TextColored(colorNosync, "NOSYNC");
                else
                    ImGui::TextColored(colorSynced, "SYNCED");

                ImGui::Text("BER   : ");
                ImGui::SameLine();
                ImGui::TextColored(viterbi1.getState() == 0 ? colorNosync : colorSynced, std::to_string(ber1).c_str());

                std::memmove(&ber_history1[0], &ber_history1[1], (200 - 1) * sizeof(float));
                ber_history1[200 - 1] = ber1;

                ImGui::PlotLines("", ber_history1, IM_ARRAYSIZE(ber_history1), 0, "", 0.0f, 1.0f, ImVec2(200, 50));
            }

            ImGui::Button("Viterbi 2", {200, 20});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (viterbi2.getState() == 0)
                    ImGui::TextColored(colorNosync, "NOSYNC");
                else
                    ImGui::TextColored(colorSynced, "SYNCED");

                ImGui::Text("BER   : ");
                ImGui::SameLine();
                ImGui::TextColored(viterbi2.getState() == 0 ? colorNosync : colorSynced, std::to_string(ber2).c_str());

                std::memmove(&ber_history2[0], &ber_history2[1], (200 - 1) * sizeof(float));
                ber_history2[200 - 1] = ber2;

                ImGui::PlotLines("", ber_history2, IM_ARRAYSIZE(ber_history2), 0, "", 0.0f, 1.0f, ImVec2(200, 50));
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
        }
        ImGui::EndGroup();

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20));

        ImGui::End();
    }

    std::string FengyunMPTDecoderModule::getID()
    {
        return "fengyun_mpt_decoder";
    }

    std::vector<std::string> FengyunMPTDecoderModule::getParameters()
    {
        return {"viterbi_outsync_after", "viterbi_ber_thresold", "soft_symbols", "invert_second_viterbi"};
    }

    std::shared_ptr<ProcessingModule> FengyunMPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<FengyunMPTDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace fengyun