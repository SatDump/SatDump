#include "module_xrit_decoder.h"
#include "logger.h"
#include "common/sathelper/correlator.h"
#include "common/sathelper/packetfixer.h"
#include "common/sathelper/derandomizer.h"
#include "common/codings/differential/nrzm.h"
#include "imgui/imgui.h"
#include "common/codings/viterbi/viterbi27.h"
#include "common/codings/correlator.h"
#include "common/codings/reedsolomon/reedsolomon.h"

#define BUFFER_SIZE 8192 * 2
#define FRAME_SIZE 1024
#define ENCODED_FRAME_SIZE 1024 * 8 * 2

// Return filesize
size_t getFilesize(std::string filepath);

namespace xrit
{
    XRITDecoderModule::XRITDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                is_bpsk(std::stoi(parameters["is_bpsk"])),
                                                                                                                                                diff_decode(std::stoi(parameters["diff_decode"])),
                                                                                                                                                viterbi(ENCODED_FRAME_SIZE / 2, viterbi::CCSDS_R2_K7_POLYS),
                                                                                                                                                d_viterbi_outsync_after(parameters.count("viterbi_outsync_after") > 0 ? std::stoi(parameters["viterbi_outsync_after"]) : 0),
                                                                                                                                                d_viterbi_ber_threasold(parameters.count("viterbi_ber_thresold") > 0 ? std::stof(parameters["viterbi_ber_thresold"]) : 0),
                                                                                                                                                stream_viterbi(d_viterbi_ber_threasold, d_viterbi_outsync_after, BUFFER_SIZE)
    {
        buffer = new uint8_t[ENCODED_FRAME_SIZE];
        viterbi_out = new uint8_t[BUFFER_SIZE * 2];
    }

    std::vector<ModuleDataType> XRITDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> XRITDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    XRITDecoderModule::~XRITDecoderModule()
    {
        delete[] buffer;
        delete[] viterbi_out;
    }

    void XRITDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);
        if (output_data_type == DATA_FILE)
        {
            data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
            d_output_files.push_back(d_output_file_hint + ".cadu");
        }

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        // Common objects
        reedsolomon::ReedSolomon rs(reedsolomon::RS223);
        diff::NRZMDiff diff;
        sathelper::Derandomizer derand;

        if (is_bpsk) // BPSK Variant using a correlator
        {
            // Correlator and shifter
            Correlator correlator(BPSK, diff_decode ? 0xfc4ef4fd0cc2df89 : 0xfca2b63db00d9794);
            sathelper::PacketFixer packetFixer;

            // Other buffers
            uint8_t frameBuffer[FRAME_SIZE];

            // Phase utils
            phase_t phase;
            bool swap;

            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                // Read a buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)buffer, ENCODED_FRAME_SIZE);
                else
                    input_fifo->read((uint8_t *)buffer, ENCODED_FRAME_SIZE);

                int pos = correlator.correlate((int8_t *)buffer, phase, swap, cor, ENCODED_FRAME_SIZE);

                locked = pos == 0; // Update locking state

                if (pos != 0 && pos < ENCODED_FRAME_SIZE) // Safety
                {
                    std::memmove(buffer, &buffer[pos], pos);

                    if (input_data_type == DATA_FILE)
                        data_in.read((char *)&buffer[ENCODED_FRAME_SIZE - pos], pos);
                    else
                        input_fifo->read((uint8_t *)&buffer[ENCODED_FRAME_SIZE - pos], pos);
                }

                // Correct phase ambiguity
                packetFixer.fixPacket(buffer, ENCODED_FRAME_SIZE, (sathelper::PhaseShift)(phase + 2), swap);

                // Viterbi
                viterbi.work((int8_t *)buffer, frameBuffer);

                // NRZ-M Decoding
                if (diff_decode)
                    diff.decode(frameBuffer, FRAME_SIZE);

                // Derandomize that frame
                derand.work(&frameBuffer[4], FRAME_SIZE - 4);

                // RS Correction
                rs.decode_interlaved(&frameBuffer[4], true, 4, errors);

                // Write it out if it's not garbage
                if (errors[0] >= 0 && errors[1] >= 0 && errors[2] >= 0 && errors[3] >= 0)
                {
                    frameBuffer[0] = 0x1a;
                    frameBuffer[1] = 0xcf;
                    frameBuffer[2] = 0xfc;
                    frameBuffer[3] = 0x1d;

                    if (output_data_type == DATA_FILE)
                        data_out.write((char *)frameBuffer, FRAME_SIZE);
                    else
                        output_fifo->write(frameBuffer, FRAME_SIZE);
                }

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                // Update module stats
                module_stats["lock_state"] = locked;
                module_stats["viterbi_ber"] = viterbi.ber() * 100;
                module_stats["rs_avg"] = (errors[0] + errors[1] + errors[2] + errors[3]) / 4;

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    std::string lock_state = locked ? "SYNCED" : "NOSYNC";
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Viterbi BER : " + std::to_string(viterbi.ber() * 100) + "%, Lock : " + lock_state);
                }
            }
        }
        else
        {
            int vout;

            while (!data_in.eof())
            {
                // Read a buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)buffer, BUFFER_SIZE);
                else
                    input_fifo->read((uint8_t *)buffer, BUFFER_SIZE);

                // Perform Viterbi decoding
                vout = stream_viterbi.work((uint8_t *)buffer, BUFFER_SIZE, viterbi_out);

                // NRZ-M Decoding
                if (diff_decode)
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
                            if (errors[0] >= 0 && errors[1] >= 0 && errors[2] >= 0 && errors[3] >= 0)
                            {
                                if (output_data_type == DATA_FILE)
                                    data_out.write((char *)&cadu, FRAME_SIZE);
                                else
                                    output_fifo->write((uint8_t *)&cadu, FRAME_SIZE);
                            }
                        }
                    }
                }

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                // Update module stats
                module_stats["viterbi_lock"] = stream_viterbi.getState();
                module_stats["viterbi_ber"] = stream_viterbi.ber() * 100;
                module_stats["deframer_lock"] = deframer.getState();
                module_stats["rs_avg"] = (errors[0] + errors[1] + errors[2] + errors[3]) / 4;

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    std::string viterbi_state = stream_viterbi.getState() == 0 ? "NOSYNC" : "SYNCED";
                    std::string deframer_state = deframer.getState() == 0 ? "NOSYNC" : (deframer.getState() == 2 || deframer.getState() == 6 ? "SYNCING" : "SYNCED");
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Viterbi : " + viterbi_state + " BER : " + std::to_string(viterbi.ber()) + ", Deframer : " + deframer_state);
                }
            }
        }

        if (output_data_type == DATA_FILE)
            data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void XRITDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("xRIT Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

        float ber = is_bpsk ? viterbi.ber() : stream_viterbi.ber();

        ImGui::BeginGroup();
        {
            // Constellation
            {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                         ImVec2(ImGui::GetCursorScreenPos().x + 200 * ui_scale, ImGui::GetCursorScreenPos().y + 200 * ui_scale),
                                         ImColor::HSV(0, 0, 0));

                if (is_bpsk)
                {
                    for (int i = 0; i < 2048; i++)
                    {
                        draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)buffer)[i] / 127.0) * 130 * ui_scale) % int(200 * ui_scale),
                                                          ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + rng.gasdev() * 14 * ui_scale) % int(200 * ui_scale)),
                                                   2 * ui_scale,
                                                   ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
                    }
                }
                else
                {
                    for (int i = 0; i < 2048; i++)
                    {
                        draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)buffer)[i * 2 + 0] / 127.0) * 100 * ui_scale) % int(200 * ui_scale),
                                                          ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + (((int8_t *)buffer)[i * 2 + 1] / 127.0) * 100 * ui_scale) % int(200 * ui_scale)),
                                                   2 * ui_scale,
                                                   ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
                    }
                }

                ImGui::Dummy(ImVec2(200 * ui_scale + 3, 200 * ui_scale + 3));
            }
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            if (is_bpsk)
            {
                ImGui::Button("Correlator", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("Corr  : ");
                    ImGui::SameLine();
                    ImGui::TextColored(locked ? IMCOLOR_SYNCED : IMCOLOR_SYNCING, UITO_C_STR(cor));

                    std::memmove(&cor_history[0], &cor_history[1], (200 - 1) * sizeof(float));
                    cor_history[200 - 1] = cor;

                    ImGui::PlotLines("", cor_history, IM_ARRAYSIZE(cor_history), 0, "", 40.0f, 64.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
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
            else
            {
                ImGui::Button("Viterbi", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("State : ");

                    ImGui::SameLine();

                    if (stream_viterbi.getState() == 0)
                        ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                    else
                        ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");

                    ImGui::Text("BER   : ");
                    ImGui::SameLine();
                    ImGui::TextColored(stream_viterbi.getState() == 0 ? IMCOLOR_NOSYNC : IMCOLOR_SYNCED, UITO_C_STR(ber));

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

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string XRITDecoderModule::getID()
    {
        return "xrit_decoder";
    }

    std::vector<std::string> XRITDecoderModule::getParameters()
    {
        return {"is_bpsk", "diff_decode", "viterbi_outsync_after", "viterbi_ber_thresold"};
    }

    std::shared_ptr<ProcessingModule> XRITDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<XRITDecoderModule>(input_file, output_file_hint, parameters);
    }
}