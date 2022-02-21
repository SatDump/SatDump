#include "module_fengyun_xeuvi.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "xeuvi_deframer.h"
#include "xeuvi_reader.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun3
{
    namespace xeuvi
    {
        FengyunXEUVIDecoderModule::FengyunXEUVIDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void FengyunXEUVIDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/XEUVI";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            int vcidFrames = 0, xeuvi_frames = 0;

            // Read buffer
            uint8_t buffer[1024];

            // Deframer
            XEUVIDeframer deframer;
            XEUVIReader reader(directory);

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 1024);

                // Extract VCID
                int vcid = buffer[5] % ((int)pow(2, 6));

                if (vcid == 5)
                {
                    std::vector<std::vector<uint8_t>> out = deframer.work(&buffer[14], 882);

                    for (std::vector<uint8_t> frameVec : out)
                    {
                        reader.work(frameVec);
                        xeuvi_frames++;
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
            logger->info("XEUVI Frames          : " + std::to_string(xeuvi_frames));
            logger->info("XEUVI Images          : " + std::to_string(reader.images_count));

            reader.writeCurrent();
        }

        void FengyunXEUVIDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("FengYun XEUVI Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string FengyunXEUVIDecoderModule::getID()
        {
            return "fengyun_xeuvi";
        }

        std::vector<std::string> FengyunXEUVIDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> FengyunXEUVIDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<FengyunXEUVIDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace virr
} // namespace fengyun