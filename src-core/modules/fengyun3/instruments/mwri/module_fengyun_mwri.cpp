#include "module_fengyun_mwri.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "mwri_deframer.h"
#include "mwri_reader.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun3
{
    namespace mwri
    {
        FengyunMWRIDecoderModule::FengyunMWRIDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void FengyunMWRIDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWRI";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            int vcidFrames = 0, mwri_frames = 0;

            // Read buffer
            uint8_t buffer[1024];

            // Deframer
            MWRIDeframer deframer;
            MWRIReader reader;

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

                    // Deframe MWRI
                    std::vector<std::vector<uint8_t>> out = deframer.work(&buffer[14], 882);

                    for (std::vector<uint8_t> frameVec : out)
                    {
                        mwri_frames++;
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

            logger->info("VCID 10 Frames        : " + std::to_string(vcidFrames));
            logger->info("MWRI Frames           : " + std::to_string(mwri_frames));
            logger->info("MWRI Lines            : " + std::to_string(reader.lines));

            logger->info("Writing images.... (Can take a while)");

            for (int i = 0; i < 10; i++)
            {
                logger->info("Channel " + std::to_string(i + 1) + "...");
                WRITE_IMAGE(reader.getChannel(i), directory + "/MWRI-" + std::to_string(i + 1) + ".png");
            }

            // Output a few nice composites as well
            logger->info("Global Composite...");
            image::Image<uint16_t> imageAll(266 * 3, reader.lines * 4, 1);
            {
                int height = reader.lines;

                // Row 1
                imageAll.draw_image(0, reader.getChannel(0), 266 * 0);
                imageAll.draw_image(0, reader.getChannel(1), 266 * 1);
                imageAll.draw_image(0, reader.getChannel(2), 266 * 2);

                // Row 2
                imageAll.draw_image(0, reader.getChannel(3), 266 * 0, height);
                imageAll.draw_image(0, reader.getChannel(4), 266 * 1, height);
                imageAll.draw_image(0, reader.getChannel(5), 266 * 2, height);

                // Row 3
                imageAll.draw_image(0, reader.getChannel(6), 266 * 0, height * 2);
                imageAll.draw_image(0, reader.getChannel(7), 266 * 1, height * 2);
                imageAll.draw_image(0, reader.getChannel(8), 266 * 2, height * 2);

                // Row 4
                imageAll.draw_image(0, reader.getChannel(9), 266 * 0, height * 3);
            }
            WRITE_IMAGE(imageAll, directory + "/MWRI-ALL.png");
        }

        void FengyunMWRIDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("FengYun MWRI Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string FengyunMWRIDecoderModule::getID()
        {
            return "fengyun_mwri";
        }

        std::vector<std::string> FengyunMWRIDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> FengyunMWRIDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<FengyunMWRIDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace virr
} // namespace fengyun