#include "module_cloudsat_cpr.h"
#include "cpr_reader.h"
#include "image/image_lut.h"
#include "image/io.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

#define BUFFER_SIZE 8192

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace cloudsat
{
    namespace cpr
    {
        CloudSatCPRDecoderModule::CloudSatCPRDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
        }

        void CloudSatCPRDecoderModule::process()
        {
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/CPR";

            CPReader reader;
            uint8_t buffer[402];

            logger->info("Demultiplexing and deframing...");

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)buffer, 402);

                reader.work(buffer);
            }

            cleanup();

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

            drawProgressBar();

            ImGui::End();
        }

        std::string CloudSatCPRDecoderModule::getID() { return "cloudsat_cpr"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> CloudSatCPRDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<CloudSatCPRDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace cpr
} // namespace cloudsat