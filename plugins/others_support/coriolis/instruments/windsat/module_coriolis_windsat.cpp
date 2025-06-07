#include "module_coriolis_windsat.h"
#include "image/io.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

// Return filesize
uint64_t getFilesize(std::string filepath);

#include "windsat_reader.h"

namespace coriolis
{
    namespace windsat
    {
        CoriolisWindSatDecoderModule::CoriolisWindSatDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
        }

        void CoriolisWindSatDecoderModule::process()
        {
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/WindSat";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            uint8_t buffer[1024];

            logger->info("Demultiplexing and deframing...");

            std::vector<std::shared_ptr<WindSatReader>> readers = {std::make_shared<WindSatReader>(1, 388),   std::make_shared<WindSatReader>(2, 490),  std::make_shared<WindSatReader>(3, 490),
                                                                   std::make_shared<WindSatReader>(4, 490),   std::make_shared<WindSatReader>(5, 812),  std::make_shared<WindSatReader>(6, 812),
                                                                   std::make_shared<WindSatReader>(7, 812),   std::make_shared<WindSatReader>(8, 977),  std::make_shared<WindSatReader>(9, 1662),
                                                                   std::make_shared<WindSatReader>(10, 1662), std::make_shared<WindSatReader>(11, 1662)};

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)buffer, 1024);

                for (int ff = 0; ff < 15; ff++)
                {
                    uint8_t *frame = &buffer[58 + ff * 64];

                    for (std::shared_ptr<WindSatReader> reader : readers)
                        reader->work(frame);
                }
            }

            for (int i = 0; i < 11; i++)
            {
                auto img = readers[i]->getImage1();
                image::save_img(img, directory + "/WindSat-" + std::to_string(i + 1));
                img = readers[i]->getImage2();
                image::save_img(img, directory + "/WindSat-" + std::to_string(i + 12));
            }

            cleanup();
        }

        void CoriolisWindSatDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Coriolis WindSat Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            drawProgressBar();

            ImGui::End();
        }

        std::string CoriolisWindSatDecoderModule::getID() { return "coriolis_windsat"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> CoriolisWindSatDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<CoriolisWindSatDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace windsat
} // namespace coriolis