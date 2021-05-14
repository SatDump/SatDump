#include "module_meteor_lrpt_decoder.h"
#include "logger.h"
#include "modules/common/sathelper/reedsolomon_233.h"
#include "modules/common/sathelper/correlator.h"
#include "modules/common/sathelper/packetfixer.h"
#include "modules/common/sathelper/derandomizer.h"
#include "modules/common/differential/nrzm.h"
#include "imgui/imgui.h"
#include "modules/common/viterbi/cc_decoder_impl.h"
#include "modules/common/repack_bits_byte.h"
#include "modules/common/utils.h"
#include "modules/common/correlator.h"

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
    }

    std::vector<ModuleDataType> METEORLRPTDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> METEORLRPTDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    METEORLRPTDecoderModule::~METEORLRPTDecoderModule()
    {
        delete[] buffer;
    }

    void METEORLRPTDecoderModule::process()
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

        // Correlator
        Correlator correlator(QPSK, diff_decode ? 0xfc4ef4fd0cc2df89 : 0xfca2b63db00d9794);

        // Viterbi, rs, etc
        sathelper::PacketFixer packetFixer;
        sathelper::Derandomizer derand;
        sathelper::ReedSolomon reedSolomon;

        // Other buffers
        uint8_t frameBuffer[FRAME_SIZE];
        uint8_t bufferh[FRAME_SIZE * 8];
        uint8_t bufferu[ENCODED_FRAME_SIZE];

        phase_t phase;
        bool swap;

        // Vectors are inverted
        fec::code::cc_decoder_impl cc_decoder_in(ENCODED_FRAME_SIZE / 2, 7, 2, {-79, -109}, 0, -1, CC_STREAMING, false);
        diff::NRZMDiff diff;

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)buffer, ENCODED_FRAME_SIZE);
            else
                input_fifo->read((uint8_t *)buffer, ENCODED_FRAME_SIZE);

            int pos = correlator.correlate((int8_t *)buffer, phase, swap, cor, ENCODED_FRAME_SIZE);

            if (pos != 0 && pos < ENCODED_FRAME_SIZE) // Safety
            {
                std::memmove(buffer, &buffer[pos], pos);
                
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)&buffer[pos], ENCODED_FRAME_SIZE - pos);
                else
                    input_fifo->read((uint8_t *)&buffer[pos], ENCODED_FRAME_SIZE - pos);
            }

            // Correct phase ambiguity
            packetFixer.fixPacket(buffer, ENCODED_FRAME_SIZE, (sathelper::PhaseShift)phase, swap);

            // Viterbi
            viterbi.decode(buffer, frameBuffer); // This gotta get removed ASAP... Gotta write a wrapper for the other one

            char_array_to_uchar((int8_t *)buffer, bufferu, ENCODED_FRAME_SIZE);
            cc_decoder_in.generic_work(bufferu, bufferh);

            // Repack
            int inbuf = 0, inbyte = 0;
            uint8_t shifter = 0;
            for (int i = 0; i < ENCODED_FRAME_SIZE / 2; i++)
            {
                shifter = shifter << 1 | bufferh[i];
                inbuf++;

                if (inbuf == 8)
                {
                    frameBuffer[inbyte] = shifter;
                    inbuf = 0;
                    inbyte++;
                }
            }

            if (diff_decode)
                diff.decode(frameBuffer, FRAME_SIZE);

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

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string lock_state = locked ? "SYNCED" : "NOSYNC";
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Viterbi BER : " + std::to_string(viterbi.GetPercentBER()) + "%, Lock : " + lock_state);
            }
        }

        data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
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