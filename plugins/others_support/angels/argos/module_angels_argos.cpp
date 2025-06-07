#include "module_angels_argos.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

#include "image/image.h"
#include "image/io.h"

// #include <iostream>

#define BUFFER_SIZE 8192

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace angels
{
    namespace argos
    {
        AngelsArgosDecoderModule::AngelsArgosDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
        }

        void AngelsArgosDecoderModule::process()
        {

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            uint64_t argos_cadu = 0, ccsds = 0, argos_ccsds = 0;
            uint8_t cadu[1024];

            logger->info("Demultiplexing and deframing...");
            logger->warn("This decoder is very WIP!");

            // std::ofstream frames_out(directory + "/argos.ccsds", std::ios::binary);
            // d_output_files.push_back(directory + "/argos.ccsds");

            image::Image fft_image(8, 4096, 5000, 1);
            int lines = 0;

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)&cadu, 1024);

                // Parse this transport frame
                int vcid = cadu[5] % (int)pow(2, 6);
                int type_marker = cadu[15];

                // ANGELs does NOT use a proper CCSDS protocol...
                if (type_marker == 17 && vcid == 1)
                {
                    argos_cadu++;

                    int cnt = cadu[32]; // Counter marker for the FFT data

                    if (cnt == 1 || cnt == 2 || cnt == 3 || cnt == 4 || cnt == 5)
                        memcpy((uint8_t *)fft_image.raw_data() + lines * 4096 + (cnt - 1) * 841, &cadu[52], cnt == 5 ? 728 : 842);

                    if (cnt == 5)
                        lines++;
                }
            }

            cleanup();
            // frames_out.close();

            fft_image.crop(0, 0, 4096, lines);
            image::save_img(fft_image, directory + "/argos_fft");

            logger->info("VCID 1 (ARGOS) Frames  : " + std::to_string(argos_cadu));
            logger->info("CCSDS Frames           : " + std::to_string(ccsds));
            logger->info("ARGOS CCSDS Frames     : " + std::to_string(argos_ccsds));

            logger->info("Writing images.... (Can take a while)");
        }

        void AngelsArgosDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Angels ARGOS Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            drawProgressBar();

            ImGui::End();
        }

        std::string AngelsArgosDecoderModule::getID() { return "angels_argos"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> AngelsArgosDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<AngelsArgosDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace argos
} // namespace angels