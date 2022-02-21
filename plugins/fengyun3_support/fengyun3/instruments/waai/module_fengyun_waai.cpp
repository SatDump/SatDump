#include "module_fengyun_waai.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "waai_deframer.h"
#include "waai_reader.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun3
{
    namespace waai
    {
        FengyunWAAIDecoderModule::FengyunWAAIDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void FengyunWAAIDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/WAAI";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            int vcidFrames = 0, waai_frames = 0;

            // Read buffer
            uint8_t buffer[1024];

            // Deframer
            WAAIDeframer deframer;
            WAAIReader reader;

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 1024);

                // Extract VCID
                int vcid = buffer[5] % ((int)pow(2, 6));

                if (vcid == 5)
                {
                    vcidFrames++;

                    // Deframe MERSI
                    std::vector<std::vector<uint8_t>> out = deframer.work(&buffer[14], 882);

                    for (std::vector<uint8_t> frameVec : out)
                    {
                        waai_frames++;
                        reader.work(frameVec);
                    }
                }

                progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            logger->info("VCID 5 Frames         : " + std::to_string(vcidFrames));
            logger->info("WAAI Frames           : " + std::to_string(waai_frames));
            logger->info("WAAI Lines            : " + std::to_string(reader.lines));

            logger->info("Writing images.... (Can take a while)");

            WRITE_IMAGE(reader.getChannel(), directory + "/WAAI.png");
        }

        void FengyunWAAIDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("FengYun WAAI Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string FengyunWAAIDecoderModule::getID()
        {
            return "fengyun_waai";
        }

        std::vector<std::string> FengyunWAAIDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> FengyunWAAIDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<FengyunWAAIDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace virr
} // namespace fengyun