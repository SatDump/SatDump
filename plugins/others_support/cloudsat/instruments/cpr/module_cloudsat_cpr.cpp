#include "module_cloudsat_cpr.h"
#include <fstream>
#include "cpr_reader.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "image/io.h"
#include "image/image_lut.h"

#define BUFFER_SIZE 8192

// Return filesize
uint64_t getFilesize(std::string filepath);

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
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
                }
            }

            data_in.close();

            logger->info("CPR Lines            : " + std::to_string(reader.lines));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            image::Image image = reader.getChannel();
            image::save_img(image, directory + "/CPR");

            image::Image clut = image::scale_lut(65536, 0, 65536, image::LUT_jet<uint16_t>());
            image::Image rain(16, image.width(), image.height(), 3);

            for (unsigned int i = 0; i < image.height() * image.width(); i++)
            {
                int index = image.get(i);
                // logger->info(index);
                rain.set(0, i, clut.get(0, index));
                rain.set(1, i, clut.get(1, index));
                rain.set(2, i, clut.get(2, index));
            }

            image::save_img(rain, directory + "/CPR-LUT");
        }

        void CloudSatCPRDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("CloudSat CPR Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

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