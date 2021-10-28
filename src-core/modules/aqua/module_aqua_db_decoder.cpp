#include "module_aqua_db_decoder.h"
#include "logger.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/codings/differential/nrzm.h"
#include "imgui/imgui.h"

#define BUFFER_SIZE (1024 * 8 * 8)

// Returns the asked bit!
template <typename T>
inline bool getBit(T data, int bit)
{
    return (data >> bit) & 1;
}

// Return filesize
size_t getFilesize(std::string filepath);

namespace aqua
{
    AquaDBDecoderModule::AquaDBDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        buffer = new uint8_t[BUFFER_SIZE];
    }

    std::vector<ModuleDataType> AquaDBDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> AquaDBDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    AquaDBDecoderModule::~AquaDBDecoderModule()
    {
        delete[] buffer;
    }

    void AquaDBDecoderModule::process()
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

        reedsolomon::ReedSolomon rs(reedsolomon::RS223);

        // I/Q Buffers
        uint8_t decodedBufI[BUFFER_SIZE / 2];
        uint8_t decodedBufQ[BUFFER_SIZE / 2];
        uint8_t bufI[(BUFFER_SIZE / 8) / 2];
        uint8_t bufQ[(BUFFER_SIZE / 8) / 2];

        // Final buffer after decoding
        uint8_t finalBuffer[(BUFFER_SIZE / 8)];

        // Bits => Bytes stuff
        uint8_t byteShifter = 0;
        int inByteShifter = 0;
        int inFinalByteBuffer;

        // 2 NRZ-M Decoders
        diff::NRZMDiff diff1, diff2;

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)buffer, BUFFER_SIZE);
            else
                input_fifo->read((uint8_t *)buffer, BUFFER_SIZE);

            // Demodulate QPSK... This is the crappy way but it works
            for (int i = 0; i < BUFFER_SIZE / 2; i++)
            {
                int8_t sample_i = clamp(*((int8_t *)&buffer[i * 2 + 1]));
                int8_t sample_q = clamp(*((int8_t *)&buffer[i * 2 + 0]));

                if (sample_i == -1 && sample_q == -1)
                {
                    decodedBufQ[i] = 0;
                    decodedBufI[i] = 0;
                }
                else if (sample_i == 1 && sample_q == -1)
                {
                    decodedBufQ[i] = 0;
                    decodedBufI[i] = 1;
                }
                else if (sample_i == 1 && sample_q == 1)
                {
                    decodedBufQ[i] = 1;
                    decodedBufI[i] = 1;
                }
                else if (sample_i == -1 && sample_q == 1)
                {
                    decodedBufQ[i] = 1;
                    decodedBufI[i] = 0;
                }
            }

            // Group symbols into bytes now, I channel
            inFinalByteBuffer = 1;
            inByteShifter = 0;
            for (int i = 0; i < BUFFER_SIZE / 2; i++)
            {
                byteShifter = byteShifter << 1 | decodedBufI[i];
                inByteShifter++;

                if (inByteShifter == 8)
                {
                    bufI[inFinalByteBuffer++] = byteShifter;
                    inByteShifter = 0;
                }
            }

            // Group symbols into bytes now, Q channel
            inFinalByteBuffer = 1;
            inByteShifter = 0;
            for (int i = 0; i < BUFFER_SIZE / 2; i++)
            {
                byteShifter = byteShifter << 1 | decodedBufQ[i];
                inByteShifter++;

                if (inByteShifter == 8)
                {
                    bufQ[inFinalByteBuffer++] = byteShifter;
                    inByteShifter = 0;
                }
            }

            // Differential decoding for both of them
            diff1.decode(bufI, (BUFFER_SIZE / 8) / 2);
            diff2.decode(bufQ, (BUFFER_SIZE / 8) / 2);

            // Interleave them back
            for (int i = 0; i < (BUFFER_SIZE / 8) / 2; i++)
            {
                finalBuffer[i * 2] = getBit<uint8_t>(bufI[i], 7) << 7 |
                                     getBit<uint8_t>(bufQ[i], 7) << 6 |
                                     getBit<uint8_t>(bufI[i], 6) << 5 |
                                     getBit<uint8_t>(bufQ[i], 6) << 4 |
                                     getBit<uint8_t>(bufI[i], 5) << 3 |
                                     getBit<uint8_t>(bufQ[i], 5) << 2 |
                                     getBit<uint8_t>(bufI[i], 4) << 1 |
                                     getBit<uint8_t>(bufQ[i], 4) << 0;
                finalBuffer[i * 2 + 1] = getBit<uint8_t>(bufI[i], 3) << 7 |
                                         getBit<uint8_t>(bufQ[i], 3) << 6 |
                                         getBit<uint8_t>(bufI[i], 2) << 5 |
                                         getBit<uint8_t>(bufQ[i], 2) << 4 |
                                         getBit<uint8_t>(bufI[i], 1) << 3 |
                                         getBit<uint8_t>(bufQ[i], 1) << 2 |
                                         getBit<uint8_t>(bufI[i], 0) << 1 |
                                         getBit<uint8_t>(bufQ[i], 0) << 0;
            }

            // Deframe that! (Integrated derand)
            std::vector<std::array<uint8_t, CADU_SIZE>> frameBuffer = deframer.work(finalBuffer, (BUFFER_SIZE / 8));

            // If we found frames, write them out
            if (frameBuffer.size() > 0)
            {
                for (std::array<uint8_t, CADU_SIZE> cadu : frameBuffer)
                {
                    // RS Correction
                    rs.decode_interlaved(&cadu[4], true, 4, errors);

                    // Write it to our output file!
                    data_out.write((char *)&cadu, CADU_SIZE);
                }
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string deframer_state = deframer.getState() == 0 ? "NOSYNC" : (deframer.getState() == 2 || deframer.getState() == 6 ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Deframer : " + deframer_state);
            }
        }

        data_out.close();
        if (output_data_type == DATA_FILE)
            data_in.close();
    }

    void AquaDBDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Aqua DB Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

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

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string AquaDBDecoderModule::getID()
    {
        return "aqua_db_decoder";
    }

    std::vector<std::string> AquaDBDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> AquaDBDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<AquaDBDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace aqua