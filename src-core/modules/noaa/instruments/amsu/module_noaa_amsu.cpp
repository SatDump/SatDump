#include "module_noaa_amsu.h"
#include "amsu_reader.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace noaa
{
    namespace amsu
    {
        NOAAAMSUDecoderModule::NOAAAMSUDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void NOAAAMSUDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AMSU";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            AMSUReader amsureader;

            while (!data_in.eof())
            {
                uint8_t buffer[104];

                data_in.read((char *)&buffer[0], 104);

                amsureader.work(buffer);

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

            logger->info("AMSU Lines:" + std::to_string(amsureader.linesA1));

            image::Image<uint16_t> compo = image::Image<uint16_t>(240, 2 * amsureader.linesA1, 1);
            image::Image<uint16_t> equcompo = image::Image<uint16_t>(240, 2 * amsureader.linesA1, 1);

            for (int i = 0; i < 15; i++)
            {
                image::Image<uint16_t> image = amsureader.getChannel(i);
                WRITE_IMAGE(image, directory + "/AMSU-" + std::to_string(i + 1) + ".png");
                compo.draw_image(0, image, (i % 8) * 30, ((int)i / 8) * (amsureader.linesA1));
                image.equalize();
                WRITE_IMAGE(image, directory + "/AMSU-" + std::to_string(i + 1) + "-EQU.png");
                equcompo.draw_image(0, image, (i % 8) * 30, ((int)i / 8) * (amsureader.linesA1));
            }

            WRITE_IMAGE(compo, directory + "/AMSU-ALL.png");
            WRITE_IMAGE(equcompo, directory + "/AMSU-ALL-EQU.png");
        }

        void NOAAAMSUDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("NOAA AMSU Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string NOAAAMSUDecoderModule::getID()
        {
            return "noaa_amsu";
        }

        std::vector<std::string> NOAAAMSUDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> NOAAAMSUDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<NOAAAMSUDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace noaa