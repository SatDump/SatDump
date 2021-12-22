#include "module_metop_ascat.h"
#include <fstream>
#include "ascat_reader.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "nlohmann/json_utils.h"
//#include "common/geodetic/projection/satellite_reprojector.h"
//#include "common/geodetic/projection/proj_file.h"
//#include "common/image/earth_curvature.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace metop
{
    namespace ascat
    {
        MetOpASCATDecoderModule::MetOpASCATDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void MetOpASCATDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/ASCAT";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer(882, true);
            ASCATReader ascatreader;
            uint64_t ascat_cadu = 0, ccsds = 0, ascat_ccsds = 0;
            uint8_t cadu[1024];

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 12 is ASCAT)
                if (vcdu.vcid == 15)
                {
                    ascat_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        ascatreader.work(pkt);
                        ascat_ccsds++;
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

            logger->info("VCID 15 (ASCAT) Frames : " + std::to_string(ascat_cadu));
            logger->info("CCSDS Frames           : " + std::to_string(ccsds));
            logger->info("ASCAT Frames           : " + std::to_string(ascat_ccsds));
            logger->info("ASCAT Channel 1 lines  : " + std::to_string(ascatreader.lines[0]));
            logger->info("ASCAT Channel 2 lines  : " + std::to_string(ascatreader.lines[1]));
            logger->info("ASCAT Channel 3 lines  : " + std::to_string(ascatreader.lines[2]));
            logger->info("ASCAT Channel 4 lines  : " + std::to_string(ascatreader.lines[3]));
            logger->info("ASCAT Channel 5 lines  : " + std::to_string(ascatreader.lines[4]));
            logger->info("ASCAT Channel 6 lines  : " + std::to_string(ascatreader.lines[5]));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            for (int i = 0; i < 6; i++)
            {
                logger->info("Channel " + std::to_string(i + 1) + "...");
                image::Image<uint16_t> img = ascatreader.getChannel(i);
                WRITE_IMAGE(img, directory + "/ASCAT-" + std::to_string(i + 1) + ".png");
                img.equalize();
                WRITE_IMAGE(img, directory + "/ASCAT-" + std::to_string(i + 1) + "-EQU.png");
            }

            image::Image<uint16_t> image1 = ascatreader.getChannel(0);
            image::Image<uint16_t> image2 = ascatreader.getChannel(1);
            image::Image<uint16_t> image3 = ascatreader.getChannel(2);
            image3.mirror(true, false);
            image::Image<uint16_t> image4 = ascatreader.getChannel(3);
            image4.mirror(true, false);
            image::Image<uint16_t> image5 = ascatreader.getChannel(4);
            image::Image<uint16_t> image6 = ascatreader.getChannel(5);
            image5.mirror(true, false);

            // Output a few nice composites as well
            logger->info("Global Composite...");
            image::Image<uint16_t> imageAll(256 * 2, ascatreader.getChannel(0).height() * 3, 1);
            {
                int height = ascatreader.getChannel(0).height();

                // Row 1
                imageAll.draw_image(0, image6, 256 * 0, 0);
                imageAll.draw_image(0, image3, 256 * 1, 0);

                // Row 2
                imageAll.draw_image(0, image5, 256 * 0, height);
                imageAll.draw_image(0, image2, 256 * 1, height);

                // Row 3
                imageAll.draw_image(0, image4, 256 * 0, height * 2);
                imageAll.draw_image(0, image1, 256 * 1, height * 2);
            }
            WRITE_IMAGE(imageAll, directory + "/ASCAT-ALL.png");

            image1.equalize();
            image2.equalize();
            image3.equalize();
            image4.equalize();
            image5.equalize();
            image6.equalize();

            logger->info("Global Equalized Composite...");
            image::Image<uint16_t> imageAllEqu(256 * 2, ascatreader.getChannel(0).height() * 3, 1);
            {
                int height = ascatreader.getChannel(0).height();

                // Row 1
                imageAll.draw_image(0, image6, 256 * 0, 0);
                imageAll.draw_image(0, image3, 256 * 1, 0);

                // Row 2
                imageAll.draw_image(0, image5, 256 * 0, height);
                imageAll.draw_image(0, image2, 256 * 1, height);

                // Row 3
                imageAll.draw_image(0, image4, 256 * 0, height * 2);
                imageAll.draw_image(0, image1, 256 * 1, height * 2);
            }
            WRITE_IMAGE(imageAll, directory + "/ASCAT-EQU-ALL.png");

            // Reproject to an equirectangular proj
            /*if (image2.height() > 0)
            {
                nlohmann::json satData = loadJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json");
                int norad = satData.contains("norad") > 0 ? satData["norad"].get<int>() : 0;
                //image4.equalize(1000);

                // Setup Projecition
                geodetic::projection::LEOScanProjectorSettings proj_settings = {
                    35,                          // Scan angle
                    35,                          // Roll offset
                    0,                           // Pitch offset
                    0,                           // Yaw offset
                    -1,                          // Time offset
                    image2.width(),              // Image width
                    true,                        // Invert scan
                    tle::getTLEfromNORAD(norad), // TLEs
                    ascatreader.timestamps[1]    // Timestamps
                };
                geodetic::projection::LEOScanProjector projector(proj_settings);
                geodetic::projection::LEOScanProjectorSettings proj_settings2 = {
                    35,                          // Scan angle
                    -35,                         // Roll offset
                    0,                           // Pitch offset
                    0,                           // Yaw offset
                    -1,                          // Time offset
                    image5.width(),              // Image width
                    true,                        // Invert scan
                    tle::getTLEfromNORAD(norad), // TLEs
                    ascatreader.timestamps[4]    // Timestamps
                };
                geodetic::projection::LEOScanProjector projector2(proj_settings2);

                {
                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                    geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/ASCAT-2.georef");
                }

                logger->info("Projected channel 2...");
                cimg_library::CImg<unsigned char> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image2, projector, 2048 * 4, 1024 * 4, 1);
                projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image5, projector2, 2048 * 4, 1024 * 4, 1, projected_image);
                WRITE_IMAGE(projected_image, directory + "/ASCAT-2-PROJ.png");
            }*/
        }

        void MetOpASCATDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("MetOp ASCAT Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string MetOpASCATDecoderModule::getID()
        {
            return "metop_ascat";
        }

        std::vector<std::string> MetOpASCATDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> MetOpASCATDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<MetOpASCATDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace mhs
} // namespace metop