#include "module_new_fengyun_ahrpt_decoder.h"
#include "logger.h"
#include "common/sathelper/reedsolomon_233.h"
#include "common/sathelper/packetfixer.h"
#include "diff.h"
#include "modules/metop/instruments/iasi/utils.h"
#include "common/ctpl/ctpl_stl.h"
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192 * 4

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun
{
    NewFengyunAHRPTDecoderModule::NewFengyunAHRPTDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                                      d_viterbi_outsync_after(std::stoi(parameters["viterbi_outsync_after"])),
                                                                                                                                                                      d_viterbi_ber_threasold(std::stof(parameters["viterbi_ber_thresold"])),
                                                                                                                                                                      d_soft_symbols(std::stoi(parameters["soft_symbols"])),
                                                                                                                                                                      d_invert_second_viterbi(std::stoi(parameters["invert_second_viterbi"])),
                                                                                                                                                                      viterbi(d_viterbi_ber_threasold, d_viterbi_outsync_after, BUFFER_SIZE / 2)
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

    std::vector<ModuleDataType> NewFengyunAHRPTDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> NewFengyunAHRPTDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    NewFengyunAHRPTDecoderModule::~NewFengyunAHRPTDecoderModule()
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

    void NewFengyunAHRPTDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
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

        // Util values
        int shift = 0;
        sathelper::PacketFixer shifter;
        bool iq_invert = false;
        int noSyncRuns = 0, viterbiNoSyncRun = 0;

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)soft_buffer, BUFFER_SIZE);
            else
                input_fifo->read((uint8_t *)soft_buffer, BUFFER_SIZE);

            shifter.fixPacket((uint8_t *)soft_buffer, BUFFER_SIZE, sathelper::DEG_0, iq_invert);

            // Deinterleave I & Q for the 2 Viterbis
            for (int i = 0; i < BUFFER_SIZE / 2; i++)
            {
                iSamples[i] = soft_buffer[(i + shift) * 2 + 0];
                qSamples[i] = d_invert_second_viterbi ? ~soft_buffer[(i + shift) * 2 + 1] : soft_buffer[(i + shift) * 2 + 1]; // Second may be inverted, FY-3C, FY-3D...
            }

            // Run Viterbi!
            vout = viterbi.work((uint8_t *)qSamples, (uint8_t *)iSamples, BUFFER_SIZE / 2, viterbi1_out, viterbi2_out);

            diffin = 0;

            // Interleave and pack output into 2 bits chunks
            if (vout > 0)
            {
                for (int y = 0; y < vout; y++)
                {
                    for (int i = 7; i >= 0; i--)
                    {
                        diff_in[diffin++] = getBit<uint8_t>(viterbi2_out[y], i) << 1 | getBit<uint8_t>(viterbi1_out[y], i);
                    }
                }

                viterbiNoSyncRun = 0;
            }

            if (viterbi.getState() == 0)
            {
                viterbiNoSyncRun++;

                if (viterbiNoSyncRun >= 10)
                {
                    if (shift == 0)
                        shift = 1;
                    else
                        shift = 0;
                    viterbiNoSyncRun = 0;
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

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string viterbi1_state = viterbi.getState() == 0 ? "NOSYNC" : "SYNCED";
                std::string viterbi2_state = viterbi.getState() == 0 ? "NOSYNC" : "SYNCED";
                std::string deframer_state = deframer.getState() == 0 ? "NOSYNC" : (deframer.getState() == 2 || deframer.getState() == 6 ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Viterbi 1 : " + viterbi1_state + " BER : " + std::to_string(viterbi.ber1()) + ", Viterbi 2 : " + viterbi2_state + " BER : " + std::to_string(viterbi.ber2()) + ", Deframer : " + deframer_state);
            }
        }

        data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    const ImColor colorNosync = ImColor::HSV(0 / 360.0, 1, 1, 1.0);
    const ImColor colorSyncing = ImColor::HSV(39.0 / 360.0, 0.93, 1, 1.0);
    const ImColor colorSynced = ImColor::HSV(113.0 / 360.0, 1, 1, 1.0);

    void NewFengyunAHRPTDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("New FengYun AHRPT Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

        float ber1 = viterbi.ber1();
        float ber2 = viterbi.ber2();

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

            ImGui::Button("Viterbi 1", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (viterbi.getState() == 0)
                    ImGui::TextColored(colorNosync, "NOSYNC");
                else
                    ImGui::TextColored(colorSynced, "SYNCED");

                ImGui::Text("BER   : ");
                ImGui::SameLine();
                ImGui::TextColored(viterbi.getState() == 0 ? colorNosync : colorSynced, UITO_C_STR(ber1));

                std::memmove(&ber_history1[0], &ber_history1[1], (200 - 1) * sizeof(float));
                ber_history1[200 - 1] = ber1;

                ImGui::PlotLines("", ber_history1, IM_ARRAYSIZE(ber_history1), 0, "", 0.0f, 1.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
            }

            ImGui::Button("Viterbi 2", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (viterbi.getState() == 0)
                    ImGui::TextColored(colorNosync, "NOSYNC");
                else
                    ImGui::TextColored(colorSynced, "SYNCED");

                ImGui::Text("BER   : ");
                ImGui::SameLine();
                ImGui::TextColored(viterbi.getState() == 0 ? colorNosync : colorSynced, UITO_C_STR(ber2));

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
                    ImGui::TextColored(colorNosync, "NOSYNC");
                else if (deframer.getState() == 2 || deframer.getState() == 6)
                    ImGui::TextColored(colorSyncing, "SYNCING");
                else
                    ImGui::TextColored(colorSynced, "SYNCED");
            }
        }
        ImGui::EndGroup();

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string NewFengyunAHRPTDecoderModule::getID()
    {
        return "new_fengyun_ahrpt_decoder";
    }

    std::vector<std::string> NewFengyunAHRPTDecoderModule::getParameters()
    {
        return {"viterbi_outsync_after", "viterbi_ber_thresold", "soft_symbols", "invert_second_viterbi"};
    }

    std::shared_ptr<ProcessingModule> NewFengyunAHRPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<NewFengyunAHRPTDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace fengyun