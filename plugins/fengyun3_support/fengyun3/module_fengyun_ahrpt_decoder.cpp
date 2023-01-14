#include "module_fengyun_ahrpt_decoder.h"
#include "logger.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/codings/rotation.h"
#include "diff.h"
#include "common/codings/randomization.h"
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun3
{
    FengyunAHRPTDecoderModule::FengyunAHRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                            d_viterbi_outsync_after(parameters["viterbi_outsync_after"].get<int>()),
                                                                                                                                            d_viterbi_ber_threasold(parameters["viterbi_ber_thresold"].get<float>()),
                                                                                                                                            d_invert_second_viterbi(parameters["invert_second_viterbi"].get<bool>()),
                                                                                                                                            viterbi1(d_viterbi_ber_threasold, d_viterbi_outsync_after, BUFFER_SIZE, true),
                                                                                                                                            viterbi2(d_viterbi_ber_threasold, d_viterbi_outsync_after, BUFFER_SIZE, true)
    {
        soft_buffer = new int8_t[BUFFER_SIZE * 2];
        q_soft_buffer = new int8_t[BUFFER_SIZE];
        i_soft_buffer = new int8_t[BUFFER_SIZE];
        viterbi1_out = new uint8_t[BUFFER_SIZE];
        viterbi2_out = new uint8_t[BUFFER_SIZE];
        diff_out = new uint8_t[BUFFER_SIZE * 20];
    }

    std::vector<ModuleDataType> FengyunAHRPTDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> FengyunAHRPTDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    FengyunAHRPTDecoderModule::~FengyunAHRPTDecoderModule()
    {
        delete[] soft_buffer;
        delete[] q_soft_buffer;
        delete[] i_soft_buffer;
        delete[] viterbi1_out;
        delete[] viterbi2_out;
        delete[] diff_out;
    }

    void FengyunAHRPTDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".cadu");

        // Configure deframer differently
        deframer.STATE_SYNCING = 8;
        deframer.STATE_SYNCED = 16;

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        // Differential
        FengyunDiff diff;
        reedsolomon::ReedSolomon rs(reedsolomon::RS223);

        uint8_t frame_buffer[1024 * 10];

        // Tests
        int shift = 0;
        bool iq_invert = true;
        bool invert_branches = false;
        int noSyncRuns = 0, viterbiNoSyncRun = 0;

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)soft_buffer, BUFFER_SIZE * 2);
            else
                input_fifo->read((uint8_t *)soft_buffer, BUFFER_SIZE * 2);

            rotate_soft(soft_buffer, BUFFER_SIZE * 2, PHASE_0, iq_invert);

            for (int i = 0; i < BUFFER_SIZE; i++)
            {
                i_soft_buffer[i] = soft_buffer[(i + shift) * 2 + 0];
                q_soft_buffer[i] = d_invert_second_viterbi ? ~soft_buffer[(i + shift) * 2 + 1] : soft_buffer[(i + shift) * 2 + 1]; // TODO SUPPORT SWAPPING
            }

#pragma omp parallel sections num_threads(2)
            {
#pragma omp section
                v1 = viterbi1.work(i_soft_buffer, BUFFER_SIZE, viterbi1_out);
#pragma omp section
                v2 = viterbi2.work(q_soft_buffer, BUFFER_SIZE, viterbi2_out);
            }
            vout = std::min(v1, v2);

            if (viterbi1.getState() == 0 || viterbi2.getState() == 0)
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

            // Perform differential decoding. Optionally invert viterbis (and not raw IQ pre-decoding, that would trigger a resync!)
            diff.work2(invert_branches ? viterbi1_out : viterbi2_out, invert_branches ? viterbi2_out : viterbi1_out, vout, diff_out);

            // Reconstruct into bytes and write to output file
            if (v1 > 0 && v2 > 0)
            {
                // Deframe / derand
                int frames = deframer.work(diff_out, vout * 2, frame_buffer);

                // Handle potential IQ swaps
                if (deframer.getState() == deframer.STATE_NOSYNC)
                {
                    noSyncRuns++;

                    if (noSyncRuns >= 10)
                    {
                        invert_branches = !invert_branches;
                        noSyncRuns = 0;
                    }
                }
                else
                {
                    noSyncRuns = 0;
                }

                for (int i = 0; i < frames; i++)
                {
                    uint8_t *cadu = &frame_buffer[i * 1024];

                    derand_ccsds(&cadu[4], 1024 - 4);

                    // RS Correction
                    rs.decode_interlaved(&cadu[4], true, 4, errors);

                    // Write it out
                    data_out.write((char *)cadu, 1024);
                }
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            // Update module stats
            module_stats["deframer_lock"] = deframer.getState() == deframer.STATE_SYNCED;
            module_stats["viterbi1_ber"] = viterbi1.ber();
            module_stats["viterbi1_lock"] = viterbi1.getState();
            module_stats["viterbi2_ber"] = viterbi2.ber();
            module_stats["viterbi2_lock"] = viterbi2.getState();
            module_stats["rs_avg"] = (errors[0] + errors[1] + errors[2] + errors[3]) / 4;

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string viterbi1_state = viterbi1.getState() == 0 ? "NOSYNC" : "SYNCED";
                std::string viterbi2_state = viterbi2.getState() == 0 ? "NOSYNC" : "SYNCED";
                std::string deframer_state = deframer.getState() == deframer.STATE_NOSYNC ? "NOSYNC" : (deframer.getState() == deframer.STATE_SYNCING ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Viterbi 1 : " + viterbi1_state + " BER : " + std::to_string(viterbi1.ber()) + ", Viterbi 2 : " + viterbi2_state + " BER : " + std::to_string(viterbi2.ber()) + ", Deframer : " + deframer_state);
            }
        }

        data_out.close();
        if (output_data_type == DATA_FILE)
            data_in.close();
    }

    void FengyunAHRPTDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("FengYun AHRPT Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

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

                if (deframer.getState() == deframer.STATE_NOSYNC)
                    ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                else if (deframer.getState() == deframer.STATE_SYNCING)
                    ImGui::TextColored(IMCOLOR_SYNCING, "SYNCING");
                else
                    ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string FengyunAHRPTDecoderModule::getID()
    {
        return "fengyun_ahrpt_decoder";
    }

    std::vector<std::string> FengyunAHRPTDecoderModule::getParameters()
    {
        return {"viterbi_outsync_after", "viterbi_ber_thresold", "soft_symbols", "invert_second_viterbi"};
    }

    std::shared_ptr<ProcessingModule> FengyunAHRPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<FengyunAHRPTDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace fengyun