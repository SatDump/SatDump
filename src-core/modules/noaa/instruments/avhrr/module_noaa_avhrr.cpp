#include "module_noaa_avhrr.h"
#include <fstream>
#include "avhrr_reader.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/image/earth_curvature.h"
#include "modules/noaa/noaa.h"
#include "common/geodetic/projection/satellite_reprojector.h"
#include "nlohmann/json_utils.h"
#include "common/geodetic/projection/proj_file.h"
#include "common/utils.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace noaa
{
    namespace avhrr
    {
        NOAAAVHRRDecoderModule::NOAAAVHRRDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void NOAAAVHRRDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AVHRR";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            AVHRRReader reader;

            uint16_t buffer[11090];

            logger->info("Demultiplexing and deframing...");

            std::vector<double> timestamps;

            time_t dayYearValue = 0; // Start of year

            {
                time_t curr_time = time(NULL);
                struct tm timeinfo_struct;
#ifdef _WIN32
                memcpy(&timeinfo_struct, gmtime(&curr_time), sizeof(struct tm));
#else
                gmtime_r(&curr_time, &timeinfo_struct);
#endif

                // Reset to be year
                timeinfo_struct.tm_mday = 0;
                timeinfo_struct.tm_wday = 0;
                timeinfo_struct.tm_yday = 0;
                timeinfo_struct.tm_mon = 0;

                dayYearValue = mktime(&timeinfo_struct);
                dayYearValue = dayYearValue - (dayYearValue % 86400);
            }

            std::vector<int> spacecraft_ids;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 11090 * 2);

                reader.work(buffer);

                // Parse timestamp
                int day_of_year = buffer[8] >> 1;
                uint64_t milliseconds = (buffer[9] & 0x7F) << 20 | (buffer[10] << 10) | buffer[11];
                double timestamp = dayYearValue + (day_of_year * 86400) + double(milliseconds) / 1000.0;
                timestamps.push_back(timestamp);

                // Parse ID
                spacecraft_ids.push_back(((buffer[6] & 0x078) >> 3) & 0x000F);

                progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            logger->info("AVHRR Lines            : " + std::to_string(reader.lines));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            cimg_library::CImg<unsigned short> image1 = reader.getChannel(0);
            cimg_library::CImg<unsigned short> image2 = reader.getChannel(1);
            cimg_library::CImg<unsigned short> image3 = reader.getChannel(2);
            cimg_library::CImg<unsigned short> image4 = reader.getChannel(3);
            cimg_library::CImg<unsigned short> image5 = reader.getChannel(4);

            logger->info("Channel 1...");
            WRITE_IMAGE(image1, directory + "/AVHRR-1.png");

            logger->info("Channel 2...");
            WRITE_IMAGE(image2, directory + "/AVHRR-2.png");

            logger->info("Channel 3...");
            WRITE_IMAGE(image3, directory + "/AVHRR-3.png");

            logger->info("Channel 4...");
            WRITE_IMAGE(image4, directory + "/AVHRR-4.png");

            logger->info("Channel 5...");
            WRITE_IMAGE(image5, directory + "/AVHRR-5.png");

            logger->info("221 Composite...");
            {
                cimg_library::CImg<unsigned short> image221(2048, reader.lines, 1, 3);
                {
                    image221.draw_image(0, 0, 0, 0, image2);
                    image221.draw_image(0, 0, 0, 1, image2);
                    image221.draw_image(0, 0, 0, 2, image1);
                }
                WRITE_IMAGE(image221, directory + "/AVHRR-RGB-221.png");
                image221.equalize(1000);
                image221.normalize(0, std::numeric_limits<unsigned short>::max());
                WRITE_IMAGE(image221, directory + "/AVHRR-RGB-221-EQU.png");
                cimg_library::CImg<unsigned short> corrected221 = image::earth_curvature::correct_earth_curvature(image221,
                                                                                                                  NOAA_ORBIT_HEIGHT,
                                                                                                                  NOAA_AVHRR_SWATH,
                                                                                                                  NOAA_AVHRR_RES);
                WRITE_IMAGE(corrected221, directory + "/AVHRR-RGB-221-EQU-CORRECTED.png");
            }

            logger->info("Equalized Ch 4...");
            {
                image4.equalize(1000);
                image4.normalize(0, std::numeric_limits<unsigned short>::max());
                WRITE_IMAGE(image4, directory + "/AVHRR-4-EQU.png");
                cimg_library::CImg<unsigned short> corrected4 = image::earth_curvature::correct_earth_curvature(image4,
                                                                                                                NOAA_ORBIT_HEIGHT,
                                                                                                                NOAA_AVHRR_SWATH,
                                                                                                                NOAA_AVHRR_RES);
                WRITE_IMAGE(corrected4, directory + "/AVHRR-4-EQU-CORRECTED.png");
            }

            // Reproject to an equirectangular proj
            if (image1.height() > 0)
            {
                //nlohmann::json satData = loadJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json");
                int norad = 0; //28654; //satData.contains("norad") > 0 ? satData["norad"].get<int>() : 0;
                image4.equalize(1000);

                // Setup Projecition, based off N19
                geodetic::projection::LEOScanProjectorSettings proj_settings = {
                    110.6,          // Scan angle
                    -0.01,          // Roll offset
                    0,              // Pitch offset
                    -3.65,          // Yaw offset
                    1,              // Time offset
                    image4.width(), // Image width
                    true,           // Invert scan
                    tle::TLE(),     // TLEs
                    timestamps      // Timestamps
                };

                {
                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                    geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/AVHRR.georef");
                }

                // Identify satellite, and apply per-sat settings...
                int scid = most_common(spacecraft_ids.begin(), spacecraft_ids.end());
                if (scid == 7) // N15
                {
                    norad = 25338;
                    logger->info("Identified NOAA-15!");
                }
                else if (scid == 13) // N18
                {
                    norad = 28654;
                    logger->info("Identified NOAA-18!");
                    proj_settings.scan_angle = 110.4;
                    proj_settings.roll_offset = -0.1;
                    proj_settings.yaw_offset = -3.2;
                }
                else if (scid == 15) // N19
                {
                    norad = 33591;
                    logger->info("Identified NOAA-19!");
                    //proj_settings.az_offset = -1.5;
                    //proj_settings.proj_scale = 2.339;
                }
                else
                {
                    logger->error("Unknwon NOAA Satellite! Only the KLM series 15, 18 and 19 are supported as others were decomissioned!");
                }

                // Load TLEs now
                proj_settings.sat_tle = tle::getTLEfromNORAD(norad);

                geodetic::projection::LEOScanProjector projector(proj_settings);

                {
                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                    geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/AVHRR.georef");
                }

                logger->info("Projected channel 4...");
                cimg_library::CImg<unsigned char> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image4, projector, 2048 * 4, 1024 * 4, 1);
                WRITE_IMAGE(projected_image, directory + "/AVHRR-4-PROJ.png");

                cimg_library::CImg<unsigned short> image221(2048, reader.lines, 1, 3);
                {
                    image221.draw_image(0, 0, 0, 0, image2);
                    image221.draw_image(0, 0, 0, 1, image2);
                    image221.draw_image(0, 0, 0, 2, image1);
                }
                image221.equalize(1000);

                logger->info("Projected channel 221...");
                projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image221, projector, 2048 * 4, 1024 * 4, 3);
                WRITE_IMAGE(projected_image, directory + "/AVHRR-RGB-221-PROJ.png");
            }
        }

        void NOAAAVHRRDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("NOAA AVHRR Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string NOAAAVHRRDecoderModule::getID()
        {
            return "noaa_avhrr";
        }

        std::vector<std::string> NOAAAVHRRDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> NOAAAVHRRDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<NOAAAVHRRDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace avhrr
} // namespace noaa