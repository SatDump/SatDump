#include "module_noaa_mhs.h"
#include "mhs_reader.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/image/image.h"
#include "nlohmann/json_utils.h"
#include "common/geodetic/projection/satellite_reprojector.h"
#include "common/geodetic/projection/proj_file.h"

#include <iostream>

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace noaa
{
    namespace mhs
    {
        NOAAMHSDecoderModule::NOAAMHSDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void NOAAMHSDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MHS";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            MHSReader mhsreader;

            while (!data_in.eof())
            {
                uint8_t buffer[104];
                std::memset(buffer, 0, 104);

                data_in.read((char *)&buffer[0], 104);

                mhsreader.work(buffer);

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

            logger->info("MHS Lines:" + std::to_string(mhsreader.line + 1));

            mhsreader.calibrate();

            image::Image<uint16_t> compo(MHS_WIDTH * 3, 2 * mhsreader.line + 1, 1);
            image::Image<uint16_t> equcompo(MHS_WIDTH * 3, 2 * mhsreader.line + 1, 1);

            for (int i = 0; i < 5; i++)
            {
                image::Image<uint16_t> image = mhsreader.getChannel(i);
                WRITE_IMAGE(image, directory + "/MHS-" + std::to_string(i + 1) + ".png");
                compo.draw_image(0, image, (i % 3) * MHS_WIDTH, ((int)i / 3) * (mhsreader.line + 1));
                image.equalize();
                WRITE_IMAGE(image, directory + "/MHS-" + std::to_string(i + 1) + "-EQU.png");
                equcompo.draw_image(0, image, (i % 3) * MHS_WIDTH, ((int)i / 3) * (mhsreader.line + 1));
            }

            WRITE_IMAGE(compo, directory + "/MHS-ALL.png");
            WRITE_IMAGE(equcompo, directory + "/MHS-ALL-EQU.png");

            // Reproject to an equirectangular proj
            if (mhsreader.line > 0)
            {
                // Get satellite info
                nlohmann::json satData = loadJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json");
                int norad = satData.contains("norad") > 0 ? satData["norad"].get<int>() : 0;

                // Setup Projecition
                std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_SCANLINE> proj_settings = geodetic::projection::makeScalineSettingsFromJSON("noaa_mhs.json");
                proj_settings->sat_tle = tle::getTLEfromNORAD(norad); // TLEs
                proj_settings->utc_timestamps = mhsreader.timestamps; // Timestamps
                geodetic::projection::LEOScanProjector projector(proj_settings);

                {
                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                    geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/MHS.georef");
                }

                for (int i = 0; i < 5; i++)
                {
                    image::Image<uint16_t> image = mhsreader.getChannel(i).equalize().normalize();
                    logger->info("Projected channel " + std::to_string(i + 1) + "...");
                    image::Image<uint8_t> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image, projector, 2048, 1024);
                    WRITE_IMAGE(projected_image, directory + "/MHS-" + std::to_string(i + 1) + "-PROJ.png");
                }
            }

            image::Image<uint8_t> rain(mhsreader.getChannel(3).width(), mhsreader.getChannel(3).height(), 3);
            std::vector<double> ch5 = mhsreader.get_calibrated_channel(4);
            std::vector<double> ch3 = mhsreader.get_calibrated_channel(3);
            image::Image<uint8_t> clut = image::scale_lut<uint8_t>(1024, 0, 100, image::LUT_jet<uint8_t>());

            for (unsigned int i = 0; i < ch5.size(); i++)
            {
                if (ch3[i] - ch5[i] > -3)
                {
                    int index = (((ch3[i] - ch5[i]) + 3) * 200) / 64;
                    if (index < 0)
                        continue;
                    if (index < 1024)
                    {
                        uint8_t color[3] = {clut[index], clut[1024 + index], clut[2048 + index]};
                        rain.draw_pixel(i % rain.width(), i / rain.width(), color);
                    }
                    else
                    {
                        uint8_t color[3] = {0};
                        rain.draw_pixel(i % rain.width(), i / rain.width(), color);
                    }
                }
            }
            WRITE_IMAGE(rain, directory + "/rain.png");
        }

        void NOAAMHSDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("NOAA MHS Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string NOAAMHSDecoderModule::getID()
        {
            return "noaa_mhs";
        }

        std::vector<std::string> NOAAMHSDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> NOAAMHSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<NOAAMHSDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace mhs
} // namespace noaa