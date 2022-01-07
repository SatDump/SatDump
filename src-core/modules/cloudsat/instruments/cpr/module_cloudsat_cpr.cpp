#include "module_cloudsat_cpr.h"
#include <fstream>
#include "cpr_reader.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace cloudsat
{
    namespace cpr
    {
        CloudSatCPRDecoderModule::CloudSatCPRDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void CloudSatCPRDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/CPR";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            CPReader reader;
            uint8_t buffer[402];

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 402);

                reader.work(buffer);

                progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            logger->info("CPR Lines            : " + std::to_string(reader.lines));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            image::Image<uint16_t> image = reader.getChannel(0);

            logger->info("Image...");
            WRITE_IMAGE(image, directory + "/CPR.png");

            image::Image<uint8_t> clut = image::scale_lut<uint8_t>(65536, 0, 65536, image::LUT_jet<uint8_t>());
            image::Image<uint8_t> rain(image.width(), image.height(), 3);

            for (unsigned int i = 0; i < image.height() * image.width(); i++)
            {
                int index = image[i];
                //logger->info(index);
                rain.channel(0)[i] = clut.channel(0)[index];
                rain.channel(1)[i] = clut.channel(1)[index];
                rain.channel(2)[i] = clut.channel(2)[index];
            }

            WRITE_IMAGE(rain, directory + "/CPR-LUT.png");
        }

        void CloudSatCPRDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("CloudSat CPR Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string CloudSatCPRDecoderModule::getID()
        {
            return "cloudsat_cpr";
        }

        std::vector<std::string> CloudSatCPRDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> CloudSatCPRDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<CloudSatCPRDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace avhrr
} // namespace noaa