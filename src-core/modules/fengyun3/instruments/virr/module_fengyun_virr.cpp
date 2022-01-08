#include "module_fengyun_virr.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "virr_deframer.h"
#include "virr_reader.h"
#include "common/image/xfr.h"
#include "imgui/imgui.h"
#include "common/image/earth_curvature.h"
#include "modules/fengyun3/fengyun3.h"
#include "nlohmann/json_utils.h"
#include "common/geodetic/projection/satellite_reprojector.h"
#include "common/geodetic/projection/proj_file.h"
#include "common/image/composite.h"
#include "common/map/leo_drawer.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun3
{
    namespace virr
    {
        FengyunVIRRDecoderModule::FengyunVIRRDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        std::string getHRPTReaderTimeStamp()
        {
            const time_t timevalue = time(0);
            std::tm *timeReadable = gmtime(&timevalue);

            std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +                                                                               // Year yyyy
                                    (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" + // Month MM
                                    (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "-" +          // Day dd
                                    (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +                // Hour HH
                                    (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min));                    // Minutes mm

            return timestamp;
        }

        void FengyunVIRRDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/VIRR";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            int vcidFrames = 0, virr_frames = 0;

            // Read buffer
            uint8_t buffer[1024];

            // Deframer
            VIRRDeframer virrDefra;

            VIRRReader reader;

            logger->info("Demultiplexing and deframing...");

            // Get satellite info
            nlohmann::json satData = loadJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json");
            int scid = satData.contains("scid") > 0 ? satData["scid"].get<int>() : 0;

            // Name the file properly
            std::string hpt_prefix = "FY3x_";

            if (scid == FY3_A_SCID)
            {
                hpt_prefix = "FY3A_";
                reader.day_offset = 14923; // Unknown, but I don't have any data to calibrate so...
            }
            else if (scid == FY3_B_SCID)
            {
                hpt_prefix = "FY3B_";
                reader.day_offset = 14923;
            }
            else if (scid == FY3_C_SCID)
            {
                hpt_prefix = "FY3C_";
                reader.day_offset = 17620;
            }

            std::string c10_filename = hpt_prefix + getHRPTReaderTimeStamp() + ".C10";
            std::ofstream output_hrpt_reader(directory + "/" + c10_filename, std::ios::binary);
            d_output_files.push_back(directory + "/" + c10_filename);

            uint8_t c10_buffer[27728];

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 1024);

                // Extract VCID
                int vcid = buffer[5] % ((int)pow(2, 6));

                if (vcid == 5)
                {
                    vcidFrames++;

                    // Deframe MERSI
                    std::vector<std::vector<uint8_t>> out = virrDefra.work(&buffer[14], 882);

                    for (std::vector<uint8_t> frameVec : out)
                    {
                        virr_frames++;
                        reader.work(frameVec);

                        // Write out C10
                        {
                            // Clean it up
                            std::fill(c10_buffer, &c10_buffer[27728], 0);

                            // Write header
                            c10_buffer[0] = 0xa1;
                            c10_buffer[1] = 0x16;
                            c10_buffer[2] = 0xfd;
                            c10_buffer[3] = 0x71;
                            c10_buffer[4] = 0x9d;
                            c10_buffer[5] = 0x83;
                            c10_buffer[6] = 0xc9;
                            c10_buffer[7] = 0x50;
                            c10_buffer[8] = 0x34;
                            c10_buffer[9] = 0x00;
                            c10_buffer[10] = 0x3d;

                            // Timestamp
                            c10_buffer[11] = 0b00101000 | (((frameVec[26044] & 0b111111) << 2 | frameVec[26045] >> 6) & 0b111);
                            c10_buffer[12] = (frameVec[26045] & 0b111111) << 2 | frameVec[26046] >> 6;
                            c10_buffer[13] = (frameVec[26046] & 0b111111) << 2 | frameVec[26047] >> 6;
                            c10_buffer[14] = (frameVec[26047] & 0b111111) << 2 | frameVec[26048] >> 6;

                            // Imagery, shifted by 2 bits
                            for (int i = 0; i < 25600 + 16; i++)
                                c10_buffer[2000 + i] = (frameVec[436 + i] & 0b111111) << 2 | frameVec[437 + i] >> 6;

                            // Last marker
                            c10_buffer[27613] = 0b0000010;

                            // Write it out
                            output_hrpt_reader.write((char *)c10_buffer, 27728);
                        }
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
            output_hrpt_reader.close();

            logger->info("VCID 5 Frames         : " + std::to_string(vcidFrames));
            logger->info("VIRR Frames           : " + std::to_string(virr_frames));
            logger->info("VIRR Lines            : " + std::to_string(reader.lines));

            logger->info("Writing images.... (Can take a while)");

            image::Image<uint16_t> image1 = reader.getChannel(0);
            image::Image<uint16_t> image2 = reader.getChannel(1);
            image::Image<uint16_t> image3 = reader.getChannel(2);
            image::Image<uint16_t> image4 = reader.getChannel(3);
            image::Image<uint16_t> image5 = reader.getChannel(4);
            image::Image<uint16_t> image6 = reader.getChannel(5);
            image::Image<uint16_t> image7 = reader.getChannel(6);
            image::Image<uint16_t> image8 = reader.getChannel(7);
            image::Image<uint16_t> image9 = reader.getChannel(8);
            image::Image<uint16_t> image10 = reader.getChannel(9);

            // Takes a while so we say how we're doing
            logger->info("Channel 1...");
            WRITE_IMAGE(image1, directory + "/VIRR-1.png");

            logger->info("Channel 2...");
            WRITE_IMAGE(image2, directory + "/VIRR-2.png");

            logger->info("Channel 3...");
            WRITE_IMAGE(image3, directory + "/VIRR-3.png");

            logger->info("Channel 4...");
            WRITE_IMAGE(image4, directory + "/VIRR-4.png");

            logger->info("Channel 5...");
            WRITE_IMAGE(image5, directory + "/VIRR-5.png");

            logger->info("Channel 6...");
            WRITE_IMAGE(image6, directory + "/VIRR-6.png");

            logger->info("Channel 7...");
            WRITE_IMAGE(image7, directory + "/VIRR-7.png");

            logger->info("Channel 8...");
            WRITE_IMAGE(image8, directory + "/VIRR-8.png");

            logger->info("Channel 9...");
            WRITE_IMAGE(image9, directory + "/VIRR-9.png");

            logger->info("Channel 10...");
            WRITE_IMAGE(image10, directory + "/VIRR-10.png");

            // Reproject to an equirectangular proj
            if (reader.lines > 0)
            {
                int norad = satData.contains("norad") > 0 ? satData["norad"].get<int>() : 0;

                // Setup Projecition
                std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_SCANLINE> proj_settings = geodetic::projection::makeScalineSettingsFromJSON("fengyun_abc_virr.json");
                proj_settings->sat_tle = tle::getTLEfromNORAD(norad); // TLEs
                proj_settings->utc_timestamps = reader.timestamps;    // Timestamps
                geodetic::projection::LEOScanProjector projector(proj_settings);

                {
                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                    geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/VIRR.georef");
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

                    std::string name = "VIRR-" + compokey.key();

                    logger->info(name + "...");
                    image::Image<uint16_t> compositeImage = image::generate_composite_from_equ<unsigned short>({image1, image2, image3, image4, image5, image6, image7, image8, image9, image10},
                                                                                                               {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
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
                        projector.setup_forward();
                        logger->info(name + "-MAP...");
                        image::Image<uint8_t> mapped_image = map::drawMapToLEO(compositeImage.to8bits(), projector);
                        WRITE_IMAGE(mapped_image, directory + "/" + name + "-MAP.png");
                    }

                    if (corrected)
                    {
                        logger->info(name + "-CORRECTED...");
                        compositeImage = image::earth_curvature::correct_earth_curvature(compositeImage,
                                                                                         FY3_ORBIT_HEIGHT,
                                                                                         FY3_VIRR_SWATH,
                                                                                         FY3_VIRR_RES);
                        WRITE_IMAGE(compositeImage, directory + "/" + name + "-CORRECTED.png");
                    }
                }
            }

            logger->info("197 True Color XFR Composite... (by ZbychuButItWasTaken)");
            {
                image::Image<uint16_t> image197truecolorxfr(2048, reader.lines, 3);
                {
                    image::Image<uint16_t> tempImage1 = image1, tempImage9 = image9, tempImage7 = image7;

                    image::xfr::XFR trueColor(26, 663, 165, 34, 999, 162, 47, 829, 165);

                    image::xfr::applyXFR(trueColor, tempImage1, tempImage9, tempImage7);

                    image197truecolorxfr.draw_image(0, tempImage1, 1);
                    image197truecolorxfr.draw_image(1, tempImage9, 0);
                    image197truecolorxfr.draw_image(2, tempImage7, -2);
                }
                WRITE_IMAGE(image197truecolorxfr, directory + "/VIRR-RGB-197-TRUECOLOR.png");
                image::Image<uint16_t> corrected197truecolorxfr = image::earth_curvature::correct_earth_curvature(image197truecolorxfr,
                                                                                                                  FY3_ORBIT_HEIGHT,
                                                                                                                  FY3_VIRR_SWATH,
                                                                                                                  FY3_VIRR_RES);
                WRITE_IMAGE(corrected197truecolorxfr, directory + "/VIRR-RGB-197-TRUECOLOR-CORRECTED.png");
                image197truecolorxfr.equalize();
                WRITE_IMAGE(image197truecolorxfr, directory + "/VIRR-RGB-197-TRUECOLOR-EQU.png");
                image::Image<uint16_t> corrected197truecolorxfrequ = image::earth_curvature::correct_earth_curvature(image197truecolorxfr,
                                                                                                                     FY3_ORBIT_HEIGHT,
                                                                                                                     FY3_VIRR_SWATH,
                                                                                                                     FY3_VIRR_RES);
                WRITE_IMAGE(corrected197truecolorxfrequ, directory + "/VIRR-RGB-197-TRUECOLOR-EQU-CORRECTED.png");
            }

            logger->info("197 Night XFR Composite... (by ZbychuButItWasTaken)");
            {
                image::Image<uint16_t> image197nightxfr(2048, reader.lines, 3);
                {
                    image::Image<uint16_t> tempImage1 = image1, tempImage9 = image9, tempImage7 = image7;

                    image::xfr::XFR trueColor(23, 610, 153, 34, 999, 162, 39, 829, 165);

                    image::xfr::applyXFR(trueColor, tempImage1, tempImage9, tempImage7);

                    image197nightxfr.draw_image(0, tempImage1, 1);
                    image197nightxfr.draw_image(1, tempImage9, 0);
                    image197nightxfr.draw_image(2, tempImage7, -2);
                }
                WRITE_IMAGE(image197nightxfr, directory + "/VIRR-RGB-197-NIGHT.png");
                image::Image<uint16_t> corrected97nightxfr = image::earth_curvature::correct_earth_curvature(image197nightxfr,
                                                                                                             FY3_ORBIT_HEIGHT,
                                                                                                             FY3_VIRR_SWATH,
                                                                                                             FY3_VIRR_RES);
                WRITE_IMAGE(corrected97nightxfr, directory + "/VIRR-RGB-197-NIGHT-CORRECTED.png");
                image197nightxfr.equalize();
                WRITE_IMAGE(image197nightxfr, directory + "/VIRR-RGB-197-NIGHT-EQU.png");
                image::Image<uint16_t> corrected97nightxfrequ = image::earth_curvature::correct_earth_curvature(image197nightxfr,
                                                                                                                FY3_ORBIT_HEIGHT,
                                                                                                                FY3_VIRR_SWATH,
                                                                                                                FY3_VIRR_RES);
                WRITE_IMAGE(corrected97nightxfrequ, directory + "/VIRR-RGB-197-NIGHT-EQU-CORRECTED.png");
            }
        }

        void FengyunVIRRDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("FengYun VIRR Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string FengyunVIRRDecoderModule::getID()
        {
            return "fengyun_virr";
        }

        std::vector<std::string> FengyunVIRRDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> FengyunVIRRDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<FengyunVIRRDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace virr
} // namespace fengyun