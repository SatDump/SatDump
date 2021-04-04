#include "module_meteor_lrpt_decoder.h"
#include "logger.h"
#include "modules/common/sathelper/reedsolomon_233.h"
#include "modules/common/sathelper/correlator.h"
#include "modules/common/sathelper/packetfixer.h"
#include "modules/common/sathelper/derandomizer.h"
#include "modules/common/differential/nrzm.h"
#include "imgui/imgui.h"

#define FRAME_SIZE 1024
#define ENCODED_FRAME_SIZE 1024 * 8 * 2

// Return filesize
size_t getFilesize(std::string filepath);

namespace meteor
{
    METEORLRPTDecoderModule::METEORLRPTDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                            diff_decode(std::stoi(parameters["diff_decode"])),
                                                                                                                                                            viterbi(ENCODED_FRAME_SIZE / 2)
    {
        buffer = new uint8_t[ENCODED_FRAME_SIZE];
        buffer_2 = new uint8_t[ENCODED_FRAME_SIZE];
    }

    METEORLRPTDecoderModule::~METEORLRPTDecoderModule()
    {
        delete[] buffer;
        delete[] buffer_2;
    }

    void METEORLRPTDecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".cadu");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        // Correlator
        sathelper::Correlator correlator;

        if (diff_decode)
        {
            // All differentially encoded sync words
            correlator.addWord((uint64_t)0xfc4ef4fd0cc2df89);
            correlator.addWord((uint64_t)0x56275254a66b45ec);
            correlator.addWord((uint64_t)0x03b10b02f33d2076);
            correlator.addWord((uint64_t)0xa9d8adab5994ba89);

            correlator.addWord((uint64_t)0xfc8df8fe0cc1ef46);
            correlator.addWord((uint64_t)0xa91ba1a859978adc);
            correlator.addWord((uint64_t)0x03720701f33e1089);
            correlator.addWord((uint64_t)0x56e45e57a6687546);
        }
        else
        {
            // All  encoded sync words
            correlator.addWord((uint64_t)0xfca2b63db00d9794);
            correlator.addWord((uint64_t)0x56fbd394daa4c1c2);
            correlator.addWord((uint64_t)0x035d49c24ff2686b);
            correlator.addWord((uint64_t)0xa9042c6b255b3e3d);

            correlator.addWord((uint64_t)0xfc51793e700e6b68);
            correlator.addWord((uint64_t)0xa9f7e368e558c2c1);
            correlator.addWord((uint64_t)0x03ae86c18ff19497);
            correlator.addWord((uint64_t)0x56081c971aa73d3e);
        }

        // Viterbi, rs, etc
        sathelper::PacketFixer packetFixer;
        sathelper::PhaseShift phaseShift;
        sathelper::Derandomizer derand;
        sathelper::ReedSolomon reedSolomon;

        bool iqinv;

        // Other buffers
        uint8_t frameBuffer[FRAME_SIZE];

        while (!data_in.eof())
        {
            // Read buffer
            data_in.read((char *)buffer, ENCODED_FRAME_SIZE);

            // Correlate
            correlator.correlate(buffer, ENCODED_FRAME_SIZE);

            // Correlator statistics
            cor = correlator.getHighestCorrelation();
            uint32_t word = correlator.getCorrelationWordNumber();
            uint32_t pos = correlator.getHighestCorrelationPosition();

            if (cor > 10)
            {
                iqinv = (word / 4) > 0;
                switch (word % 4)
                {
                case 0:
                    phaseShift = sathelper::PhaseShift::DEG_0;
                    break;

                case 1:
                    phaseShift = sathelper::PhaseShift::DEG_90;
                    break;

                case 2:
                    phaseShift = sathelper::PhaseShift::DEG_180;
                    break;

                case 3:
                    phaseShift = sathelper::PhaseShift::DEG_270;
                    break;

                default:
                    break;
                }

                if (pos != 0)
                {
                    shiftWithConstantSize(buffer, pos, ENCODED_FRAME_SIZE);
                    uint32_t offset = ENCODED_FRAME_SIZE - pos;

                    data_in.read((char *)buffer_2, pos);

                    for (int i = offset; i < ENCODED_FRAME_SIZE; i++)
                    {
                        buffer[i] = buffer_2[i - offset];
                    }
                }
                else
                {
                }

                // Correct phase ambiguity
                packetFixer.fixPacket(buffer, ENCODED_FRAME_SIZE, phaseShift, iqinv);

                // Viterbi
                viterbi.decode(buffer, frameBuffer);

                if (diff_decode)
                    diff::nrzm_decode(frameBuffer, FRAME_SIZE);

                // Derandomize that frame
                derand.work(&frameBuffer[4], FRAME_SIZE - 4);

                // RS Correction
                for (int i = 0; i < 4; i++)
                {
                    reedSolomon.deinterleave(&frameBuffer[4], rsWorkBuffer, i, 4);
                    errors[i] = reedSolomon.decode_rs8(rsWorkBuffer);
                    reedSolomon.interleave(rsWorkBuffer, &frameBuffer[4], i, 4);
                }

                // Write it out if it's not garbage
                if (errors[0] >= 0 && errors[1] >= 0 && errors[2] >= 0 && errors[3] >= 0)
                {
                    data_out.put(0x1a);
                    data_out.put(0xcf);
                    data_out.put(0xfc);
                    data_out.put(0x1d);
                    data_out.write((char *)&frameBuffer[4], FRAME_SIZE - 4);
                }
            }

            progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string lock_state = locked ? "SYNCED" : "NOSYNC";
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Viterbi BER : " + std::to_string(viterbi.GetPercentBER()) + "%, Lock : " + lock_state);
            }
        }

        data_out.close();
        data_in.close();
    }

    void METEORLRPTDecoderModule::shiftWithConstantSize(uint8_t *arr, int pos, int length)
    {
        for (int i = 0; i < length - pos; i++)
        {
            arr[i] = arr[pos + i];
        }
    }

    const ImColor colorNosync = ImColor::HSV(0 / 360.0, 1, 1, 1.0);
    const ImColor colorSyncing = ImColor::HSV(39.0 / 360.0, 0.93, 1, 1.0);
    const ImColor colorSynced = ImColor::HSV(113.0 / 360.0, 1, 1, 1.0);

    void METEORLRPTDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("METEOR LRPT Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

        float ber = viterbi.GetPercentBER() / 100.0f;

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
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 + (((int8_t *)buffer)[i * 2 + 0] / 127.0) * 100) % 200,
                                                      ImGui::GetCursorScreenPos().y + (int)(100 + (((int8_t *)buffer)[i * 2 + 1] / 127.0) * 100) % 200),
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
            ImGui::Button("Correlator", {200, 20});
            {
                ImGui::Text("Corr  : ");
                ImGui::SameLine();
                ImGui::TextColored(locked ? colorSynced : colorSyncing, std::to_string(cor).c_str());

                std::memmove(&cor_history[0], &cor_history[1], (200 - 1) * sizeof(float));
                cor_history[200 - 1] = cor;

                ImGui::PlotLines("", cor_history, IM_ARRAYSIZE(cor_history), 0, "", 0.0f, 50.0f, ImVec2(200, 50));
            }

            ImGui::Spacing();

            ImGui::Button("Viterbi", {200, 20});
            {
                ImGui::Text("BER   : ");
                ImGui::SameLine();
                ImGui::TextColored(ber < 0.22 ? colorSynced : colorNosync, std::to_string(ber).c_str());

                std::memmove(&ber_history[0], &ber_history[1], (200 - 1) * sizeof(float));
                ber_history[200 - 1] = ber;

                ImGui::PlotLines("", ber_history, IM_ARRAYSIZE(ber_history), 0, "", 0.0f, 1.0f, ImVec2(200, 50));
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

    std::string METEORLRPTDecoderModule::getID()
    {
        return "meteor_lrpt_decoder";
    }

    std::vector<std::string> METEORLRPTDecoderModule::getParameters()
    {
        return {"diff_decode"};
    }

    std::shared_ptr<ProcessingModule> METEORLRPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<METEORLRPTDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace meteor