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
#include "common/geodetic/projection/proj_file.h"
#include "common/utils.h"
#include "common/image/composite.h"
#include "common/image/brightness_contrast.h"
#include "common/map/leo_drawer.h"
#include <ctime>

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace meteor
{
    namespace msumr
    {
        METEORMSUMRDecoderModule::METEORMSUMRDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
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

            image::Image<uint16_t> image1 = reader.getChannel(0);
            image::Image<uint16_t> image2 = reader.getChannel(1);
            image::Image<uint16_t> image3 = reader.getChannel(2);
            image::Image<uint16_t> image4 = reader.getChannel(3);
            image::Image<uint16_t> image5 = reader.getChannel(4);
            image::Image<uint16_t> image6 = reader.getChannel(5);

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
            WRITE_IMAGE(image6, directory + "/MSU-MR-6.png");

            // Reproject to an equirectangular proj
            if (reader.lines > 0)
            {
                int norad = 0;
                //int norad = satData.contains("norad") > 0 ? satData["norad"].get<int>() : 0;

                image::Image<uint16_t> image321(1572, reader.lines, 3);
                {
                    image321.draw_image(0, image3);
                    image321.draw_image(1, image2);
                    image321.draw_image(2, image1);
                }

                image::brightness_contrast(image321, 0.179 * 2, 0.253 * 2, 3);

                // Setup Projecition, tuned for 2-2
                std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_SCANLINE> proj_settings = geodetic::projection::makeScalineSettingsFromJSON("meteor_m2_msumr.json"); // Init it with something

                // Identify satellite, and apply per-sat settings...
                int msumr_serial_number = most_common(msumr_ids.begin(), msumr_ids.end());
                if (msumr_serial_number == 0) // METEOR-M 2, weirdly enough it has ID 0
                {
                    norad = 40069;
                    logger->info("Identified METEOR-M 2!");
                    proj_settings = geodetic::projection::makeScalineSettingsFromJSON("meteor_m2_msumr.json");

                    // Also in moscow time...
                    //for (double &timestamp : timestamps)
                    //    timestamp -= 3 * 3600;
                }
                else if (msumr_serial_number == 1) // METEOR-M 2-1... Launch failed of course...
                {
                    norad = 0;
                    logger->error("Identified METEOR-M 2-1! But that is not supposed to happen...");
                    proj_settings = geodetic::projection::makeScalineSettingsFromJSON("meteor_m2-2_msumr.json"); // Whatever, just in case
                }
                else if (msumr_serial_number == 2) // METEOR-M 2-2
                {
                    norad = 44387;
                    logger->info("Identified METEOR-M 2-2!");
                    proj_settings = geodetic::projection::makeScalineSettingsFromJSON("meteor_m2-2_msumr.json");
                }
                else
                {
                    logger->error("Unknwon METEOR Satellite!");
                }

                // Load TLEs now
                proj_settings->sat_tle = tle::getTLEfromNORAD(norad);
                proj_settings->utc_timestamps = timestamps;

                geodetic::projection::LEOScanProjector projector(proj_settings);

                {
                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                    geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/MSU-MR.georef");
                }

                // Generate composites
                for (const nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> &compokey : d_parameters["composites"].items())
                {
                    nlohmann::json compositeDef = compokey.value();

                    // Not required here
                    //std::vector<int> requiredChannels = compositeDef["channels"].get<std::vector<int>>();

                    std::string expression = compositeDef["expression"].get<std::string>();
                    bool corrected = compositeDef.count("corrected") > 0 ? compositeDef["corrected"].get<bool>() : false;
                    bool mapped = compositeDef.count("mapped") > 0 ? compositeDef["mapped"].get<bool>() : false;
                    bool projected = compositeDef.count("projected") > 0 ? compositeDef["projected"].get<bool>() : false;

                    std::string name = "MSU-MR-" + compokey.key();

                    logger->info(name + "...");
                    image::Image<uint16_t> compositeImage = image::generate_composite_from_equ<unsigned short>({image1, image2, image3, image4, image5},
                                                                                                               {1, 2, 3, 4, 5},
                                                                                                               expression,
                                                                                                               compositeDef);

                    WRITE_IMAGE(compositeImage, directory + "/" + name + ".png");

                    if (projected)
                    {
                        logger->info(name + "-PROJ...");
                        image::Image<uint8_t> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(compositeImage, projector, 2048 * 4, 1024 * 4, compositeImage.channels());
                        WRITE_IMAGE(projected_image, directory + "/" + name + "-PROJ.png");
                    }

                    if (mapped)
                    {
                        projector.setup_forward(90, 10, 20);
                        logger->info(name + "-MAP...");
                        image::Image<uint8_t> mapped_image = map::drawMapToLEO(compositeImage.to8bits(), projector);
                        WRITE_IMAGE(mapped_image, directory + "/" + name + "-MAP.png");
                    }

                    if (corrected)
                    {
                        logger->info(name + "-CORRECTED...");
                        compositeImage = image::earth_curvature::correct_earth_curvature(compositeImage,
                                                                                         METEOR_ORBIT_HEIGHT,
                                                                                         METEOR_MSUMR_SWATH,
                                                                                         METEOR_MSUMR_RES);
                        WRITE_IMAGE(compositeImage, directory + "/" + name + "-CORRECTED.png");
                    }
                }
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

        std::shared_ptr<ProcessingModule> METEORMSUMRDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<METEORMSUMRDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace msumr
} // namespace meteor