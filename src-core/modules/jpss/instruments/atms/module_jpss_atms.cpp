#include "module_jpss_atms.h"
#include <fstream>
#include "atms_reader.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "nlohmann/json_utils.h"
#include "common/geodetic/projection/satellite_reprojector.h"
#include "common/geodetic/projection/proj_file.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace jpss
{
    namespace atms
    {
        JPSSATMSDecoderModule::JPSSATMSDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                        npp_mode(parameters["npp_mode"].get<bool>())
        {
            if (npp_mode)
            {
                cadu_size = 1024;
                mpdu_size = 884;
            }
            else
            {
                cadu_size = 1279;
                mpdu_size = 0;
            }
        }

        void JPSSATMSDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/ATMS";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            logger->info("Demultiplexing and deframing...");

            // Read buffer
            uint8_t cadu[1024];

            // Counters
            uint64_t atms_cadu = 0, ccsds = 0, atms_ccsds = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer = ccsds::ccsds_1_0_1024::Demuxer(mpdu_size, false);

            ATMSReader reader;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, cadu_size);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 1 is ATMS)
                if (vcdu.vcid == 1)
                {
                    atms_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 528)
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 528)
                        {
                            atms_ccsds++;
                            reader.work(pkt);
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

            logger->info("VCID 1 (ATMS) Frames   : " + std::to_string(atms_cadu));
            logger->info("CCSDS Frames           : " + std::to_string(ccsds));
            logger->info("ATMS CCSDS Frames      : " + std::to_string(atms_ccsds));
            logger->info("ATMS Lines             : " + std::to_string(reader.lines));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            image::Image<uint16_t> image1 = reader.getImage(0);
            image::Image<uint16_t> image2 = reader.getImage(1);
            image::Image<uint16_t> image3 = reader.getImage(2);
            image::Image<uint16_t> image4 = reader.getImage(3);
            image::Image<uint16_t> image5 = reader.getImage(4);
            image::Image<uint16_t> image6 = reader.getImage(5);
            image::Image<uint16_t> image7 = reader.getImage(6);
            image::Image<uint16_t> image8 = reader.getImage(7);
            image::Image<uint16_t> image9 = reader.getImage(8);
            image::Image<uint16_t> image10 = reader.getImage(9);
            image::Image<uint16_t> image11 = reader.getImage(10);
            image::Image<uint16_t> image12 = reader.getImage(11);
            image::Image<uint16_t> image13 = reader.getImage(12);
            image::Image<uint16_t> image14 = reader.getImage(13);
            image::Image<uint16_t> image15 = reader.getImage(14);
            image::Image<uint16_t> image16 = reader.getImage(15);
            image::Image<uint16_t> image17 = reader.getImage(16);
            image::Image<uint16_t> image18 = reader.getImage(17);
            image::Image<uint16_t> image19 = reader.getImage(18);
            image::Image<uint16_t> image20 = reader.getImage(19);
            image::Image<uint16_t> image21 = reader.getImage(20);
            image::Image<uint16_t> image22 = reader.getImage(21);

            image1.equalize();
            image2.equalize();
            image3.equalize();
            image4.equalize();
            image5.equalize();
            image6.equalize();
            image7.equalize();
            image8.equalize();
            image9.equalize();
            image10.equalize();
            image11.equalize();
            image12.equalize();
            image13.equalize();
            image14.equalize();
            image15.equalize();
            image16.equalize();
            image17.equalize();
            image18.equalize();
            image19.equalize();
            image20.equalize();
            image21.equalize();
            image22.equalize();

            logger->info("Channel 1...");
            WRITE_IMAGE(image1, directory + "/ATMS-1.png");

            logger->info("Channel 2...");
            WRITE_IMAGE(image2, directory + "/ATMS-2.png");

            logger->info("Channel 3...");
            WRITE_IMAGE(image3, directory + "/ATMS-3.png");

            logger->info("Channel 4...");
            WRITE_IMAGE(image4, directory + "/ATMS-4.png");

            logger->info("Channel 5...");
            WRITE_IMAGE(image5, directory + "/ATMS-5.png");

            logger->info("Channel 6...");
            WRITE_IMAGE(image6, directory + "/ATMS-6.png");

            logger->info("Channel 7...");
            WRITE_IMAGE(image7, directory + "/ATMS-7.png");

            logger->info("Channel 8...");
            WRITE_IMAGE(image8, directory + "/ATMS-8.png");

            logger->info("Channel 9...");
            WRITE_IMAGE(image9, directory + "/ATMS-9.png");

            logger->info("Channel 10...");
            WRITE_IMAGE(image10, directory + "/ATMS-10.png");

            logger->info("Channel 11...");
            WRITE_IMAGE(image11, directory + "/ATMS-11.png");

            logger->info("Channel 12...");
            WRITE_IMAGE(image12, directory + "/ATMS-12.png");

            logger->info("Channel 13...");
            WRITE_IMAGE(image13, directory + "/ATMS-13.png");

            logger->info("Channel 14...");
            WRITE_IMAGE(image14, directory + "/ATMS-14.png");

            logger->info("Channel 15...");
            WRITE_IMAGE(image15, directory + "/ATMS-15.png");

            logger->info("Channel 16...");
            WRITE_IMAGE(image16, directory + "/ATMS-16.png");

            logger->info("Channel 17...");
            WRITE_IMAGE(image17, directory + "/ATMS-17.png");

            logger->info("Channel 18...");
            WRITE_IMAGE(image18, directory + "/ATMS-18.png");

            logger->info("Channel 19...");
            WRITE_IMAGE(image19, directory + "/ATMS-19.png");

            logger->info("Channel 20...");
            WRITE_IMAGE(image20, directory + "/ATMS-20.png");

            logger->info("Channel 21...");
            WRITE_IMAGE(image21, directory + "/ATMS-21.png");

            logger->info("Channel 22...");
            WRITE_IMAGE(image22, directory + "/ATMS-22.png");

            // Output a few nice composites as well
            logger->info("Global Composite...");
            image::Image<uint16_t> imageAll(96 * 11, image1.height() * 2, 1);
            {
                // Row 1
                imageAll.draw_image(0, image1, 96 * 0);
                imageAll.draw_image(0, image2, 96 * 1);
                imageAll.draw_image(0, image3, 96 * 2);
                imageAll.draw_image(0, image4, 96 * 3);
                imageAll.draw_image(0, image5, 96 * 4);
                imageAll.draw_image(0, image6, 96 * 5);
                imageAll.draw_image(0, image7, 96 * 6);
                imageAll.draw_image(0, image8, 96 * 7);
                imageAll.draw_image(0, image9, 96 * 8);
                imageAll.draw_image(0, image10, 96 * 9);
                imageAll.draw_image(0, image11, 96 * 10);

                // Row 2
                imageAll.draw_image(0, image12, 96 * 0, image1.height());
                imageAll.draw_image(0, image13, 96 * 1, image1.height());
                imageAll.draw_image(0, image14, 96 * 2, image1.height());
                imageAll.draw_image(0, image15, 96 * 3, image1.height());
                imageAll.draw_image(0, image16, 96 * 4, image1.height());
                imageAll.draw_image(0, image17, 96 * 5, image1.height());
                imageAll.draw_image(0, image18, 96 * 6, image1.height());
                imageAll.draw_image(0, image19, 96 * 7, image1.height());
                imageAll.draw_image(0, image20, 96 * 8, image1.height());
                imageAll.draw_image(0, image21, 96 * 9, image1.height());
                imageAll.draw_image(0, image22, 96 * 10, image1.height());
            }
            WRITE_IMAGE(imageAll, directory + "/ATMS-ALL.png");

            logger->info("346 Composite...");
            image::Image<uint16_t> image346(96, image1.height(), 3);
            {
                image346.draw_image(0, image3);
                image346.draw_image(1, image4);
                image346.draw_image(2, image6);
            }
            image346.equalize();
            WRITE_IMAGE(image346, directory + "/ATMS-RGB-346.png");

            logger->info("335 Composite...");
            image::Image<uint16_t> image335(96, image1.height(), 3);
            {
                image335.draw_image(0, image3);
                image335.draw_image(1, image3);
                image335.draw_image(2, image5);
            }
            image335.equalize();
            WRITE_IMAGE(image335, directory + "/ATMS-RGB-335.png");

            logger->info("4.3.17 Composite...");
            image::Image<uint16_t> image4317(96, image1.height(), 3);
            {
                image4317.draw_image(0, image4);
                image4317.draw_image(1, image3);
                image4317.draw_image(2, image17);
            }
            image4317.equalize();
            WRITE_IMAGE(image4317, directory + "/ATMS-RGB-4.3.17.png");

            logger->info("445 Composite...");
            image::Image<uint16_t> image445(96, image1.height(), 3);
            {
                image445.draw_image(0, image4);
                image445.draw_image(1, image4);
                image445.draw_image(2, image5);
            }
            image445.equalize();
            WRITE_IMAGE(image445, directory + "/ATMS-RGB-445.png");

            logger->info("4.4.17 Composite...");
            image::Image<uint16_t> image4417(96, image1.height(), 3);
            {
                image4417.draw_image(0, image4);
                image4417.draw_image(1, image4);
                image4417.draw_image(2, image17);
            }
            image4417.equalize();
            WRITE_IMAGE(image4417, directory + "/ATMS-RGB-4.4.17.png");

            logger->info("4.16.17 Composite...");
            image::Image<uint16_t> image41617(96, image1.height(), 3);
            {
                image41617.draw_image(0, image4);
                image41617.draw_image(1, image16);
                image41617.draw_image(2, image17);
            }
            image41617.equalize();
            WRITE_IMAGE(image41617, directory + "/ATMS-RGB-4.16.17.png");

            logger->info("3.4.17 Composite...");
            image::Image<uint16_t> image3417(96, image1.height(), 3);
            {
                image3417.draw_image(0, image3);
                image3417.draw_image(1, image4);
                image3417.draw_image(2, image17);
            }
            image3417.equalize();
            WRITE_IMAGE(image3417, directory + "/ATMS-RGB-3.4.17.png");

            logger->info("5.5.17 Composite...");
            image::Image<uint16_t> image5517(96, image1.height(), 3);
            {
                image5517.draw_image(0, image5);
                image5517.draw_image(1, image5);
                image5517.draw_image(2, image17);
            }
            image5517.equalize();
            WRITE_IMAGE(image5517, directory + "/ATMS-RGB-5.5.17.png");

            logger->info("6.4.17 Composite...");
            image::Image<uint16_t> image6417(96, image1.height(), 3);
            {
                image6417.draw_image(0, image6);
                image6417.draw_image(1, image4);
                image6417.draw_image(2, image17);
            }
            image6417.equalize();
            WRITE_IMAGE(image6417, directory + "/ATMS-RGB-6.4.17.png");

            logger->info("16.4.17 Composite...");
            image::Image<uint16_t> image16417(96, image1.height(), 3);
            {
                image16417.draw_image(0, image16);
                image16417.draw_image(1, image4);
                image16417.draw_image(2, image17);
            }
            image16417.equalize();
            WRITE_IMAGE(image16417, directory + "/ATMS-RGB-16.4.17.png");

            logger->info("17.16.6 Composite...");
            image::Image<uint16_t> image17166(96, image1.height(), 3);
            {
                image17166.draw_image(0, image17);
                image17166.draw_image(1, image16);
                image17166.draw_image(2, image6);
            }
            image17166.equalize();
            WRITE_IMAGE(image17166, directory + "/ATMS-RGB-17.16.6.png");

            // Output a few nice composites as well
            logger->info("Global Composite...");
            image::Image<uint16_t> imageRgbAll(96 * 6, image1.height() * 2, 3);
            {
                // Row 1
                imageRgbAll.draw_image(0, image346, 96 * 0);
                imageRgbAll.draw_image(0, image335, 96 * 1);
                imageRgbAll.draw_image(0, image4317, 96 * 2);
                imageRgbAll.draw_image(0, image445, 96 * 3);
                imageRgbAll.draw_image(0, image4417, 96 * 4);
                imageRgbAll.draw_image(0, image41617, 96 * 5);

                // Row 2
                imageRgbAll.draw_image(0, image3417, 96 * 0, image1.height());
                imageRgbAll.draw_image(0, image5517, 96 * 1, image1.height());
                imageRgbAll.draw_image(0, image6417, 96 * 2, image1.height());
                imageRgbAll.draw_image(0, image16417, 96 * 3, image1.height());
                imageRgbAll.draw_image(0, image17166, 96 * 4, image1.height());
            }
            WRITE_IMAGE(imageRgbAll, directory + "/ATMS-RGB-ALL.png");

            // Reproject to an equirectangular proj
            if (reader.lines > 0)
            {
                // Get satellite info
                nlohmann::json satData = loadJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json");
                int norad = satData.contains("norad") > 0 ? satData["norad"].get<int>() : 0;

                // Setup Projecition
                std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_SCANLINE> proj_settings = geodetic::projection::makeScalineSettingsFromJSON("jpss_atms.json");
                proj_settings->sat_tle = tle::getTLEfromNORAD(norad); // TLEs
                proj_settings->utc_timestamps = reader.timestamps;    // Timestamps
                geodetic::projection::LEOScanProjector projector(proj_settings);

                {
                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                    geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/ATMS.georef");
                }

                for (int i = 0; i < 22; i++)
                {
                    image::Image<uint16_t> image = reader.getImage(i);
                    image.equalize();
                    logger->info("Projected channel " + std::to_string(i + 1) + "...");
                    image::Image<uint8_t> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image, projector, 2048, 1024);
                    WRITE_IMAGE(projected_image, directory + "/ATMS-" + std::to_string(i + 1) + "-PROJ.png");
                }

                image::Image<uint16_t> image3417(96, image1.height(), 3);
                {
                    image3417.draw_image(0, image3);
                    image3417.draw_image(1, image4);
                    image3417.draw_image(2, image17);
                }
                image3417.equalize();

                image::Image<uint8_t> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image3417, projector, 2048, 1024, 3);
                WRITE_IMAGE(projected_image, directory + "/ATMS-RGB-3.4.17-PROJ.png");
            }
        }

        void JPSSATMSDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("JPSS ATMS Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string JPSSATMSDecoderModule::getID()
        {
            return "jpss_atms";
        }

        std::vector<std::string> JPSSATMSDecoderModule::getParameters()
        {
            return {"npp_mode"};
        }

        std::shared_ptr<ProcessingModule> JPSSATMSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<JPSSATMSDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace atms
} // namespace jpss