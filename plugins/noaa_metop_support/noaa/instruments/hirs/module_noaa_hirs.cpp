#include "module_noaa_hirs.h"
#include "hirs_reader.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace noaa
{
    namespace hirs
    {
        NOAAHIRSDecoderModule::NOAAHIRSDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void NOAAHIRSDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/HIRS";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            HIRSReader hirsreader;

            while (!data_in.eof())
            {
                uint8_t buffer[104];

                data_in.read((char *)&buffer[0], 104);

                hirsreader.work(buffer);

                progress = data_in.tellg();

                if (time(NULL) % 2 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("HIRS Lines:" + std::to_string(hirsreader.line));

            image::Image<uint16_t> compo = image::Image<uint16_t>(280, 4 * hirsreader.line, 1);
            image::Image<uint16_t> equcompo = image::Image<uint16_t>(280, 4 * hirsreader.line, 1);

            for (int i = 0; i < 20; i++)
            {
                image::Image<uint16_t> image = hirsreader.getChannel(i);
                for (int j = 0; j < (int)image.size(); j++)
                {
                    image[j] = image[j] * 8;
                }
                WRITE_IMAGE(image, directory + "/HIRS-" + std::to_string(i + 1) + ".png");
                //logger->debug(std::to_string(i)+" ," + std::to_string(hirsreader.line));
                compo.draw_image(0, image, (i % 5) * 56, ((int)i / 5) * hirsreader.line);
                image.equalize();
                WRITE_IMAGE(image, directory + "/HIRS-" + std::to_string(i + 1) + "-EQU.png");
                equcompo.draw_image(0, image, (i % 5) * 56, ((int)i / 5) * hirsreader.line);
            }

            WRITE_IMAGE(compo, directory + "/HIRS-ALL.png");
            WRITE_IMAGE(equcompo, directory + "/HIRS-ALL-EQU.png");
        }

        void NOAAHIRSDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("NOAA HIRS Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string NOAAHIRSDecoderModule::getID()
        {
            return "noaa_hirs";
        }

        std::vector<std::string> NOAAHIRSDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> NOAAHIRSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<NOAAHIRSDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace hirs
} // namespace noaa