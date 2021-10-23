#include "module_fengyun_windrad.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include <iostream>
#include "windrad_deframer.h"
#include "windrad_reader.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun3
{
    namespace windrad
    {
        FengyunWindRADDecoderModule::FengyunWindRADDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void FengyunWindRADDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/WindRAD";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            int vcidFrames = 0, windrad_frames = 0;

            // Read buffer
            uint8_t buffer[1024];

            if (!std::filesystem::exists(directory + "/C-Band"))
                std::filesystem::create_directory(directory + "/C-Band");
            if (!std::filesystem::exists(directory + "/Ku-Band"))
                std::filesystem::create_directory(directory + "/Ku-Band");

            // Deframer
            WindRADDeframer1 deframer1;
            WindRADDeframer2 deframer2;
            WindRADReader reader1(758, "", directory + "/C-Band"), reader2(1065, "", directory + "/Ku-Band");

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 1024);

                // Extract VCID
                int vcid = buffer[5] % ((int)pow(2, 6));

                if (vcid == 10)
                {
                    vcidFrames++;

                    // Deframe WindRAD 1
                    std::vector<std::vector<uint8_t>> out = deframer1.work(&buffer[14], 882);

                    for (std::vector<uint8_t> frameVec : out)
                    {
                        windrad_frames++;
                        reader1.work(frameVec);
                    }
                }

                if (vcid == 18)
                {
                    vcidFrames++;

                    // Deframe WindRAD 2
                    std::vector<std::vector<uint8_t>> out = deframer2.work(&buffer[14], 882);

                    for (std::vector<uint8_t> frameVec : out)
                    {
                        windrad_frames++;
                        reader2.work(frameVec);
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

            logger->info("VCID 10 Frames         : " + std::to_string(vcidFrames));
            logger->info("WindRAD Frames         : " + std::to_string(windrad_frames));
            logger->info("WindRAD C-Band Frames  : " + std::to_string(reader1.imgCount));
            logger->info("WindRAD Ku-Band Frames : " + std::to_string(reader2.imgCount));
        }

        void FengyunWindRADDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("FengYun WindRAD Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string FengyunWindRADDecoderModule::getID()
        {
            return "fengyun_windrad";
        }

        std::vector<std::string> FengyunWindRADDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> FengyunWindRADDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<FengyunWindRADDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace virr
} // namespace fengyun