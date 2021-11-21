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

            for (double &time : mhsreader.timestamps)
            {
                time_t tttime = time;
                std::tm *timeReadable = gmtime(&tttime);
                std::string timestampr = std::to_string(timeReadable->tm_year + 1900) + "/" +
                                         (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "/" +
                                         (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + " " +
                                         (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + ":" +
                                         (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + ":" +
                                         (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));

                logger->info(std::to_string(time) + " " + timestampr);
            }

            cimg_library::CImg<unsigned short> compo = cimg_library::CImg(MHS_WIDTH * 3, 2 * mhsreader.line + 1, 1, 1);
            cimg_library::CImg<unsigned short> equcompo = cimg_library::CImg(MHS_WIDTH * 3, 2 * mhsreader.line + 1, 1, 1);

            for (int i = 0; i < 5; i++)
            {
                cimg_library::CImg<unsigned short> image = mhsreader.getChannel(i);
                WRITE_IMAGE(image, directory + "/MHS-" + std::to_string(i + 1) + ".png");
                compo.draw_image((i % 3) * MHS_WIDTH, ((int)i / 3) * (mhsreader.line + 1), image);
                image.equalize(1000);
                WRITE_IMAGE(image, directory + "/MHS-" + std::to_string(i + 1) + "-EQU.png");
                equcompo.draw_image((i % 3) * MHS_WIDTH, ((int)i / 3) * (mhsreader.line + 1), image);
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
                    cimg_library::CImg<unsigned short> image = mhsreader.getChannel(i).equalize(1000).normalize(0, 65535);
                    logger->info("Projected channel " + std::to_string(i + 1) + "...");
                    cimg_library::CImg<unsigned char> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image, projector, 2048, 1024);
                    WRITE_IMAGE(projected_image, directory + "/MHS-" + std::to_string(i + 1) + "-PROJ.png");
                }
            }

            cimg_library::CImg<unsigned char> rain(mhsreader.getChannel(3).width(), mhsreader.getChannel(3).height(), 1, 3, 0);
            cimg_library::CImg<double> ch5 = mhsreader.get_calibrated_channel(4);
            cimg_library::CImg<double> ch3 = mhsreader.get_calibrated_channel(3);
            cimg_library::CImg<unsigned char> clut = image::generate_LUT(1024, 0, 100, cimg_library::CImg<unsigned char>::jet_LUT256(), true);

            for (unsigned int i = 0; i < ch5.size(); i++)
            {
                if (ch3[i] - ch5[i] > -3)
                {
                    int index = (((ch3[i] - ch5[i]) + 3) * 200) / 64;
                    if (index < 1024)
                    {
                        const unsigned char color[3] = {*clut.data(index, 0, 0, 0), *clut.data(index, 0, 0, 1), *clut.data(index, 0, 0, 2)};
                        rain.draw_point(i % rain.width(), i / rain.width(), 0, color, 1.0f);
                    }
                    else
                    {
                        const unsigned char color[3] = {0, 0, 0};
                        rain.draw_point(i % rain.width(), i / rain.width(), 0, color, 1.0f);
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