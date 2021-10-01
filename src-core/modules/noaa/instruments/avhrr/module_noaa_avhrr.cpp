#include "module_noaa_avhrr.h"
#include <fstream>
#include "avhrr_reader.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/image/earth_curvature.h"
#include "modules/noaa/noaa.h"
#include "common/projection/leo_to_equirect.h"
#include "nlohmann/json_utils.h"

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
                nlohmann::json satData = loadJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json");
                int norad = satData.contains("norad") > 0 ? satData["norad"].get<int>() : 0;
                //image4.equalize(1000);

                // Setup Projecition
                projection::LEOScanProjector projector({
                    2,                           // Pixel offset
                    2050,                        // Correction swath
                    1,                           // Instrument res
                    800,                         // Orbit height
                    NOAA_AVHRR_SWATH,            // Instrument swath
                    2.399,                       // Scale
                    -2.9,                        // Az offset
                    0,                           // Tilt
                    0.3,                         // Time offset
                    image4.width(),              // Image width
                    true,                        // Invert scan
                    tle::getTLEfromNORAD(28654), // TLEs
                    timestamps                   // Timestamps
                });

                logger->info("Projected channel 4...");
                cimg_library::CImg<unsigned char> projected_image = projection::projectLEOToEquirectangularMapped(image4, projector, 2048 * 20, 1024 * 20, 1);
                WRITE_IMAGE(projected_image, directory + "/AVHRR-4-PROJ.png");

                cimg_library::CImg<unsigned short> image321(2048, reader.lines, 1, 3);
                {
                    image321.draw_image(0, 0, 0, 0, image3);
                    image321.draw_image(0, 0, 0, 1, image2);
                    image321.draw_image(0, 0, 0, 2, image1);
                }

                logger->info("Projected channel 321...");
                projected_image = projection::projectLEOToEquirectangularMapped(image321, projector, 2048 * 4, 1024 * 4, 3);
                WRITE_IMAGE(projected_image, directory + "/AVHRR-RGB-321-PROJ.png");
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