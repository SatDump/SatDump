#include "module_saral_decoder.h"
#include "logger.h"
#include "common/sathelper/reedsolomon_233.h"
#include "common/sathelper/correlator.h"
#include "common/sathelper/packetfixer.h"
#include "common/sathelper/derandomizer.h"
#include "common/differential/nrzm.h"
#include "imgui/imgui.h"
#include <cmath>

#define FRAME_SIZE 1024
#define ENCODED_FRAME_SIZE 1024 * 8 * 2

// Return filesize
size_t getFilesize(std::string filepath);

namespace saral
{
    SaralDecoderModule::SaralDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                  viterbi(ENCODED_FRAME_SIZE / 2)
    {
        buffer = new uint8_t[ENCODED_FRAME_SIZE];
        buffer_2 = new uint8_t[ENCODED_FRAME_SIZE];
    }

    SaralDecoderModule::~SaralDecoderModule()
    {
        delete[] buffer;
        delete[] buffer_2;
    }

    void SaralDecoderModule::process()
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

        // Diff decoder
        diff::NRZMDiff diff_dec;

        // All differentially encoded sync words
        correlator.addWord((uint64_t)0xfc4ef4fd0cc2df89);
        correlator.addWord((uint64_t)0x56275254a66b45ec);
        correlator.addWord((uint64_t)0x03b10b02f33d2076);
        correlator.addWord((uint64_t)0xa9d8adab5994ba89);

        correlator.addWord((uint64_t)0xfc8df8fe0cc1ef46);
        correlator.addWord((uint64_t)0xa91ba1a859978adc);
        correlator.addWord((uint64_t)0x03720701f33e1089);
        correlator.addWord((uint64_t)0x56e45e57a6687546);

        // Viterbi, rs, etc
        sathelper::PacketFixer packetFixer;
        sathelper::Derandomizer derand;
        sathelper::ReedSolomon reedSolomon;

        // Other buffers
        uint8_t frameBuffer[FRAME_SIZE];

        // Other variables
        sathelper::PhaseShift phaseShift;
        bool iqinv;

        while (!data_in.eof())
        {
            // Read buffer
            data_in.read((char *)buffer, ENCODED_FRAME_SIZE);

            // Correlate less if we're locked to go faster
            if (!locked)
                correlator.correlate(buffer, ENCODED_FRAME_SIZE);
            {
                correlator.correlate(buffer, ENCODED_FRAME_SIZE / 64);
                if (correlator.getHighestCorrelationPosition() != 0)
                {
                    correlator.correlate(buffer, ENCODED_FRAME_SIZE);
                    if (correlator.getHighestCorrelationPosition() > 30)
                        locked = false;
                }
            }

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

                diff_dec.decode(frameBuffer, FRAME_SIZE);

                // RS Correction
                for (int i = 0; i < 4; i++)
                {
                    reedSolomon.deinterleave(&frameBuffer[4], rsWorkBuffer, i, 4);
                    errors[i] = reedSolomon.decode_ccsds(rsWorkBuffer);
                    reedSolomon.interleave(rsWorkBuffer, &frameBuffer[4], i, 4);
                }

                // Derandomize that frame
                derand.work(&frameBuffer[4], FRAME_SIZE - 4);

                // Write it out if it's not garbage
                if (cor > 50)
                    locked = true;

                if (locked)
                {
                    //data_out_total += FRAME_SIZE;
                    data_out.put(0x1a);
                    data_out.put(0xcf);
                    data_out.put(0xfc);
                    data_out.put(0x1d);
                    data_out.write((char *)&frameBuffer[4], FRAME_SIZE - 4);
                }
            }
            else
            {
                locked = false;
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

    void SaralDecoderModule::shiftWithConstantSize(uint8_t *arr, int pos, int length)
    {
        for (int i = 0; i < length - pos; i++)
        {
            arr[i] = arr[pos + i];
        }
    }

    const ImColor colorNosync = ImColor::HSV(0 / 360.0, 1, 1, 1.0);
    const ImColor colorSyncing = ImColor::HSV(39.0 / 360.0, 0.93, 1, 1.0);
    const ImColor colorSynced = ImColor::HSV(113.0 / 360.0, 1, 1, 1.0);

    void SaralDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Saral Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

        float ber = viterbi.GetPercentBER() / 100.0f;

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
                ImGui::TextColored(locked ? colorSynced : colorSyncing, UITO_C_STR(cor));

                std::memmove(&cor_history[0], &cor_history[1], (200 - 1) * sizeof(float));
                cor_history[200 - 1] = cor;

                ImGui::PlotLines("", cor_history, IM_ARRAYSIZE(cor_history), 0, "", 0.0f, 50.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
            }

            ImGui::Spacing();

            ImGui::Button("Viterbi", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("BER   : ");
                ImGui::SameLine();
                ImGui::TextColored(ber < 0.22 ? colorSynced : colorNosync, UITO_C_STR(ber));

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
                        ImGui::TextColored(colorNosync, "%i ", i);
                    else if (errors[i] > 0)
                        ImGui::TextColored(colorSyncing, "%i ", i);
                    else
                        ImGui::TextColored(colorSynced, "%i ", i);
                }
            }
        }
        ImGui::EndGroup();

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string SaralDecoderModule::getID()
    {
        return "saral_decoder";
    }

    std::vector<std::string> SaralDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> SaralDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<SaralDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace proba