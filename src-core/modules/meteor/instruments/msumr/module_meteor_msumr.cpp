#include "module_meteor_msumr.h"
#include <fstream>
#include "modules/meteor/simpledeframer.h"
#include "logger.h"
#include <filesystem>
#include "msumr_reader.h"
#include "imgui/imgui.h"
#include "common/image/earth_curvature.h"
#include "modules/meteor/meteor.h"
#include "nlohmann/json_utils.h"
#include "common/geodetic/projection/satellite_reprojector.h"
#include "common/image/brightness_contrast.h"
#include "common/image/image.h"
#include "common/geodetic/projection/proj_file.h"
#include "common/utils.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace meteor
{
    namespace msumr
    {
        METEORMSUMRDecoderModule::METEORMSUMRDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void METEORMSUMRDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MSU-MR";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            uint8_t buffer[1024];

            std::vector<uint8_t> msumrData;

            // MSU-MR data
            SimpleDeframer<uint64_t, 64, 11850 * 8, 0x0218A7A392DD9ABF, 10> msumrDefra;

            MSUMRReader reader;

            logger->info("Demultiplexing and deframing...");

            time_t currentDay = time(0);
            time_t dayValue = currentDay - (currentDay % 86400); // Requires the day to be known from another source

            std::vector<double> timestamps;

            std::vector<int> msumr_ids;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 1024);

                // Extract MSU-MR
                msumrData.insert(msumrData.end(), &buffer[23 - 1], &buffer[23 - 1] + 238);
                msumrData.insert(msumrData.end(), &buffer[279 - 1], &buffer[279 - 1] + 238);
                msumrData.insert(msumrData.end(), &buffer[535 - 1], &buffer[535 - 1] + 238);
                msumrData.insert(msumrData.end(), &buffer[791 - 1], &buffer[791 - 1] + 234);

                // Deframe
                std::vector<std::vector<uint8_t>> msumr_frames = msumrDefra.work(msumrData);

                for (std::vector<uint8_t> frame : msumr_frames)
                {
                    reader.work(frame.data());

                    double timestamp = dayValue + (frame[8] - 3.0) * 3600.0 + (frame[9]) * 60.0 + (frame[10] + 0.0) + double(frame[11] / 255.0);
                    timestamps.push_back(timestamp);

                    msumr_ids.push_back(frame[12] >> 4);

                    /*
                    time_t tttime = timestamp;
                    std::tm *timeReadable = gmtime(&tttime);
                    std::string timestampr = std::to_string(timeReadable->tm_year + 1900) + "/" +
                                             (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "/" +
                                             (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + " " +
                                             (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + ":" +
                                             (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + ":" +
                                             (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));

                    logger->info(std::to_string(timestamp) + " " + timestampr);
                    */
                }

                msumrData.clear();

                progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            logger->info("MSU-MR Lines          : " + std::to_string(reader.lines));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            cimg_library::CImg<unsigned short> image1 = reader.getChannel(0);
            cimg_library::CImg<unsigned short> image2 = reader.getChannel(1);
            cimg_library::CImg<unsigned short> image3 = reader.getChannel(2);
            cimg_library::CImg<unsigned short> image4 = reader.getChannel(3);
            cimg_library::CImg<unsigned short> image5 = reader.getChannel(4);
            cimg_library::CImg<unsigned short> image6 = reader.getChannel(5);

            logger->info("Channel 1...");
            WRITE_IMAGE(image1, directory + "/MSU-MR-1.png");

            logger->info("Channel 2...");
            WRITE_IMAGE(image2, directory + "/MSU-MR-2.png");

            logger->info("Channel 3...");
            WRITE_IMAGE(image3, directory + "/MSU-MR-3.png");

            logger->info("Channel 4...");
            WRITE_IMAGE(image4, directory + "/MSU-MR-4.png");

            logger->info("Channel 5...");
            WRITE_IMAGE(image5, directory + "/MSU-MR-5.png");

            logger->info("Channel 6...");
            WRITE_IMAGE(image5, directory + "/MSU-MR-6.png");

            logger->info("221 Composite...");
            {
                cimg_library::CImg<unsigned short> image221(1572, reader.lines, 1, 3);
                {
                    image221.draw_image(0, 0, 0, 0, image2);
                    image221.draw_image(0, 0, 0, 1, image2);
                    image221.draw_image(0, 0, 0, 2, image1);
                }
                WRITE_IMAGE(image221, directory + "/MSU-MR-RGB-221.png");
                image221.equalize(1000);
                image221.normalize(0, std::numeric_limits<unsigned short>::max());
                WRITE_IMAGE(image221, directory + "/MSU-MR-RGB-221-EQU.png");
                cimg_library::CImg<unsigned short> corrected221 = image::earth_curvature::correct_earth_curvature(image221,
                                                                                                                  METEOR_ORBIT_HEIGHT,
                                                                                                                  METEOR_MSUMR_SWATH,
                                                                                                                  METEOR_MSUMR_RES);
                WRITE_IMAGE(corrected221, directory + "/MSU-MR-RGB-221-EQU-CORRECTED.png");
            }

            logger->info("321 Composite...");
            {
                cimg_library::CImg<unsigned short> image321(1572, reader.lines, 1, 3);
                {
                    image321.draw_image(0, 0, 0, 0, image3);
                    image321.draw_image(0, 0, 0, 1, image2);
                    image321.draw_image(0, 0, 0, 2, image1);
                }
                cimg_library::CImg<unsigned short> image321_contrast = image321;
                WRITE_IMAGE(image321, directory + "/MSU-MR-RGB-321.png");
                image321.equalize(1000);
                image321.normalize(0, std::numeric_limits<unsigned short>::max());
                WRITE_IMAGE(image321, directory + "/MSU-MR-RGB-321-EQU.png");
                cimg_library::CImg<unsigned short> corrected321 = image::earth_curvature::correct_earth_curvature(image321,
                                                                                                                  METEOR_ORBIT_HEIGHT,
                                                                                                                  METEOR_MSUMR_SWATH,
                                                                                                                  METEOR_MSUMR_RES);
                WRITE_IMAGE(corrected321, directory + "/MSU-MR-RGB-321-EQU-CORRECTED.png");

                image::brightness_contrast(image321_contrast, 0.179 * 2, 0.253 * 2, 3);
                WRITE_IMAGE(image321_contrast, directory + "/MSU-MR-RGB-321-CONT.png");
                cimg_library::CImg<unsigned short> corrected321_contrast = image::earth_curvature::correct_earth_curvature(image321_contrast,
                                                                                                                           METEOR_ORBIT_HEIGHT,
                                                                                                                           METEOR_MSUMR_SWATH,
                                                                                                                           METEOR_MSUMR_RES);
                WRITE_IMAGE(corrected321_contrast, directory + "/MSU-MR-RGB-321-CONT-CORRECTED.png");
            }

            logger->info("Equalized Ch 4...");
            {
                image::linear_invert(image4);
                image4.equalize(1000);
                image4.normalize(0, std::numeric_limits<unsigned short>::max());
                WRITE_IMAGE(image4, directory + "/MSU-MR-4-EQU.png");
                cimg_library::CImg<unsigned short> corrected4 = image::earth_curvature::correct_earth_curvature(image4,
                                                                                                                METEOR_ORBIT_HEIGHT,
                                                                                                                METEOR_MSUMR_SWATH,
                                                                                                                METEOR_MSUMR_RES);
                WRITE_IMAGE(corrected4, directory + "/MSU-MR-4-EQU-CORRECTED.png");
            }

            logger->info("Equalized Ch 5...");
            {
                image5.equalize(1000);
                image5.normalize(0, std::numeric_limits<unsigned short>::max());
                WRITE_IMAGE(image5, directory + "/MSU-MR-5-EQU.png");
                cimg_library::CImg<unsigned short> corrected5 = image::earth_curvature::correct_earth_curvature(image5,
                                                                                                                METEOR_ORBIT_HEIGHT,
                                                                                                                METEOR_MSUMR_SWATH,
                                                                                                                METEOR_MSUMR_RES);
                WRITE_IMAGE(corrected5, directory + "/MSU-MR-5-EQU-CORRECTED.png");
            }

            // Reproject to an equirectangular proj
            {
                int norad = 0;
                //int norad = satData.contains("norad") > 0 ? satData["norad"].get<int>() : 0;

                cimg_library::CImg<unsigned short> image321(1572, reader.lines, 1, 3);
                {
                    image321.draw_image(0, 0, 0, 0, image3);
                    image321.draw_image(0, 0, 0, 1, image2);
                    image321.draw_image(0, 0, 0, 2, image1);
                }

                image::brightness_contrast(image321, 0.179 * 2, 0.253 * 2, 3);

                // Setup Projecition, tuned for 2-2
                std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_SCANLINE> proj_settings = std::make_shared<geodetic::projection::LEOScanProjectorSettings_SCANLINE>(
                    110.6,          // Scan angle
                    -0.35,          // Roll offset
                    0,              // Pitch offset
                    -2.8,           // Yaw offset
                    0.2,            // Time offset
                    image1.width(), // Image width
                    true,           // Invert scan
                    tle::TLE(),     // TLEs
                    timestamps      // Timestamps
                );

                // Identify satellite, and apply per-sat settings...
                int msumr_serial_number = most_common(msumr_ids.begin(), msumr_ids.end());
                if (msumr_serial_number == 0) // METEOR-M 2, weirdly enough it has ID 0
                {
                    norad = 40069;
                    logger->info("Identified METEOR-M 2!");
                    proj_settings->scan_angle = 110.6;
                    proj_settings->roll_offset = 2.5;
                    proj_settings->yaw_offset = -2.8;
                    proj_settings->time_offset = 0.2;
                }
                else if (msumr_serial_number == 1) // METEOR-M 2-1... Launch failed of course...
                {
                    norad = 0;
                    logger->info("Identified METEOR-M 2-1! But that is not supposed to happen...");
                }
                else if (msumr_serial_number == 2) // METEOR-M 2-2
                {
                    norad = 44387;
                    logger->info("Identified METEOR-M 2-2!");
                }
                else
                {
                    logger->error("Unknwon METEOR Satellite!");
                }

                // Load TLEs now
                proj_settings->sat_tle = tle::getTLEfromNORAD(norad);

                geodetic::projection::LEOScanProjector projector(proj_settings);

                {
                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                    geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/MSU-MR.georef");
                }

                logger->info("Projected RGB 321...");
                cimg_library::CImg<unsigned char> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image321, projector, 2048 * 4, 1024 * 4, 3);
                WRITE_IMAGE(projected_image, directory + "/MSU-MR-RGB-321-PROJ.png");

                logger->info("Projected Channel 4...");
                projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image4, projector, 2048 * 4, 1024 * 4, 1);
                WRITE_IMAGE(projected_image, directory + "/MSU-MR-4-PROJ.png");
            }
        }

        void METEORMSUMRDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("METEOR MSU-MR Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string METEORMSUMRDecoderModule::getID()
        {
            return "meteor_msumr";
        }

        std::vector<std::string> METEORMSUMRDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> METEORMSUMRDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<METEORMSUMRDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace msumr
} // namespace meteor