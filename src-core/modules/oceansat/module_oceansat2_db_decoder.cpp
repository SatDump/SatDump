#include "module_oceansat2_db_decoder.h"
#include "logger.h"
#include "modules/common/differential/generic.h"
#include "imgui/imgui.h"
#include "oceansat2_deframer.h"
#include "oceansat2_derand.h"

#define BUFFER_SIZE 8192

// Returns the asked bit!
template <typename T>
inline bool getBit(T data, int bit)
{
    return (data >> bit) & 1;
}

// Return filesize
size_t getFilesize(std::string filepath);

namespace oceansat
{
    Oceansat2DBDecoderModule::Oceansat2DBDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        buffer = new int8_t[BUFFER_SIZE];
        frame_count = 0;
    }

    Oceansat2DBDecoderModule::~Oceansat2DBDecoderModule()
    {
        delete[] buffer;
    }

    void Oceansat2DBDecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".ocm", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".ocm");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".ocm");

        time_t lastTime = 0;

        // I/Q Buffers
        uint8_t decodedBuffer[BUFFER_SIZE];
        uint8_t diffedBuffer[BUFFER_SIZE];

        // Final buffer after decoding
        uint8_t finalBuffer[BUFFER_SIZE];

        // Bits => Bytes stuff
        uint8_t byteShifter;
        int inByteShifter = 0;
        int inFinalByteBuffer;

        diff::GenericDiff diffdecoder(4);

        Oceansat2Deframer deframer;
        Oceansat2Derand derand;

        while (!data_in.eof())
        {
            // Read buffer
            data_in.read((char *)buffer, BUFFER_SIZE);

            // Demodulate DQPSK... This is the crappy way but it works
            for (int i = 0; i < BUFFER_SIZE / 2; i++)
            {
                bool a = buffer[i * 2] > 0;
                bool b = buffer[i * 2 + 1] > 0;

                if (a)
                {
                    if (b)
                        decodedBuffer[i] = 0x0;
                    else
                        decodedBuffer[i] = 0x3;
                }
                else
                {
                    if (b)
                        decodedBuffer[i] = 0x1;
                    else
                        decodedBuffer[i] = 0x2;
                }
            }

            int diff_count = diffdecoder.work(decodedBuffer, BUFFER_SIZE / 2, diffedBuffer);

            inFinalByteBuffer = 0;
            inByteShifter = 0;
            for (int i = 0; i < diff_count; i++)
            {
                byteShifter = byteShifter << 1 | getBit<uint8_t>(diffedBuffer[i], 1);
                inByteShifter++;

                if (inByteShifter == 8)
                {
                    finalBuffer[inFinalByteBuffer++] = byteShifter;
                    inByteShifter = 0;
                }
            }

            std::vector<std::vector<uint8_t>> frames = deframer.work(finalBuffer, inFinalByteBuffer);

            frame_count += frames.size();

            for (std::vector<uint8_t> frame : frames)
            {
                derand.work(frame.data());
                data_out.write((char *)frame.data(), 92220);
            }

            progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Frames : " + std::to_string(frame_count));
            }
        }

        data_out.close();
        data_in.close();
    }

    const ImColor colorNosync = ImColor::HSV(0 / 360.0, 1, 1, 1.0);
    const ImColor colorSyncing = ImColor::HSV(39.0 / 360.0, 0.93, 1, 1.0);
    const ImColor colorSynced = ImColor::HSV(113.0 / 360.0, 1, 1, 1.0);

    void Oceansat2DBDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Oceansat-2 DB Decoder", NULL, window ? NULL : NOWINDOW_FLAGS );

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
            ImGui::Button("Deframer", {200, 20});
            {
                ImGui::Text("Frames : ");

                ImGui::SameLine();

                ImGui::TextColored(ImColor::HSV(113.0 / 360.0, 1, 1, 1.0), std::to_string(frame_count).c_str());
            }
        }
        ImGui::EndGroup();

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20));

        ImGui::End();
    }

    std::string Oceansat2DBDecoderModule::getID()
    {
        return "oceansat2_db_decoder";
    }

    std::vector<std::string> Oceansat2DBDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> Oceansat2DBDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<Oceansat2DBDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace aqua