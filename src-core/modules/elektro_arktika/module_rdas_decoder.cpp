#include "module_rdas_decoder.h"
#include "logger.h"
#include "common/codings/differential/nrzm.h"
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192

// Returns the asked bit!
template <typename T>
inline bool getBit(T data, int bit)
{
    return (data >> bit) & 1;
}

// Return filesize
size_t getFilesize(std::string filepath);

namespace elektro_arktika
{
    RDASDecoderModule::RDASDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        buffer = new int8_t[BUFFER_SIZE];
    }

    RDASDecoderModule::~RDASDecoderModule()
    {
        delete[] buffer;
    }

    void RDASDecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".cadu");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        diff::NRZMDiff diff;

        // Final buffer after decoding
        uint8_t finalBuffer[BUFFER_SIZE];

        // Bits => Bytes stuff
        uint8_t byteShifter = 0;
        int inByteShifter = 0;
        int byteShifted = 0;

        while (!data_in.eof())
        {
            // Read buffer
            data_in.read((char *)buffer, BUFFER_SIZE);

            // Group symbols into bytes now, I channel
            inByteShifter = 0;
            byteShifted = 0;

            for (int i = 0; i < BUFFER_SIZE; i++)
            {
                byteShifter = byteShifter << 1 | (buffer[i] > 0);
                inByteShifter++;

                if (inByteShifter == 8)
                {
                    finalBuffer[byteShifted++] = byteShifter;
                    inByteShifter = 0;
                }
            }

            // Differential decoding for both of them
            diff.decode(finalBuffer, BUFFER_SIZE / 8);

            // Deframe that! (Integrated derand)
            std::vector<std::array<uint8_t, ccsds::ccsds_1_0_1024::CADU_SIZE>> frameBuffer = deframer.work(finalBuffer, (BUFFER_SIZE / 8));

            // If we found frames, write them out
            if (frameBuffer.size() > 0)
            {
                for (std::array<uint8_t, ccsds::ccsds_1_0_1024::CADU_SIZE> cadu : frameBuffer)
                {
                    data_out.write((char *)&cadu, ccsds::ccsds_1_0_1024::CADU_SIZE);
                }
            }

            progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string deframer_state = deframer.getState() == 0 ? "NOSYNC" : (deframer.getState() == 2 || deframer.getState() == 6 ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Deframer : " + deframer_state);
            }
        }

        data_out.close();
        data_in.close();
    }

    void RDASDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("ELEKTRO / ARKTIKA RDAS Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

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
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (buffer[i] / 127.0) * 100 * ui_scale) % int(200 * ui_scale),
                                                      ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + rng.gasdev() * 6 * ui_scale) % int(200 * ui_scale)),
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
        }
        ImGui::EndGroup();

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string RDASDecoderModule::getID()
    {
        return "elektro_arktika_rdas_decoder";
    }

    std::vector<std::string> RDASDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> RDASDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<RDASDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace elektro