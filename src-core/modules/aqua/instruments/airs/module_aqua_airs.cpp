#include "module_aqua_airs.h"
#include <fstream>
#include "airs_reader.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/geodetic/projection/satellite_reprojector.h"
#include "common/geodetic/projection/proj_file.h"
#include "modules/eos/eos.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace aqua
{
    namespace airs
    {
        AquaAIRSDecoderModule::AquaAIRSDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void AquaAIRSDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AIRS";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            // Read buffer
            uint8_t cadu[1024];

            // Counters
            uint64_t airs_cadu = 0, ccsds = 0, airs_ccsds = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer = ccsds::ccsds_1_0_1024::Demuxer();

            // Readers
            AIRSReader airs_reader;

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 35 is AIRS)
                if (vcdu.vcid == 35)
                {
                    airs_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 404)
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 404)
                        {
                            airs_reader.work(pkt);
                            airs_ccsds++;
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

            logger->info("VCID 35 (AIRS) Frames : " + std::to_string(airs_cadu));
            logger->info("CCSDS Frames          : " + std::to_string(ccsds));
            logger->info("AIRS Frames           : " + std::to_string(airs_ccsds));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            for (int i = 0; i < 4; i++)
            {
                logger->info("HD Channel " + std::to_string(i + 1) + "...");
                WRITE_IMAGE(airs_reader.getHDChannel(i), directory + "/AIRS-HD-" + std::to_string(i + 1) + ".png");
            }

            // There nearly 3000 channels... So we write that in a specific folder not to fill up the main one
            if (!std::filesystem::exists(directory + "/Channels"))
                std::filesystem::create_directory(directory + "/Channels");

            for (int i = 0; i < 2666; i++)
            {
                logger->info("Channel " + std::to_string(i + 1) + "...");
                WRITE_IMAGE(airs_reader.getChannel(i), directory + "/Channels/AIRS-" + std::to_string(i + 1) + ".png");
            }

            // Output a few nice composites as well
            logger->info("Global Composite...");
            int all_width_count = 100;
            int all_height_count = 27;
            image::Image<uint16_t> imageAll(90 * all_width_count, airs_reader.getChannel(0).height() * all_height_count, 1);
            {
                int height = airs_reader.getChannel(0).height();

                for (int row = 0; row < all_height_count; row++)
                {
                    for (int column = 0; column < all_width_count; column++)
                    {
                        if (row * all_width_count + column >= 2666)
                            break;

                        imageAll.draw_image(0, airs_reader.getChannel(row * all_width_count + column), 90 * column, height * row);
                    }
                }
            }
            WRITE_IMAGE(imageAll, directory + "/AIRS-ALL.png");

            logger->info("HD 221 Composite...");
            image::Image<uint16_t> image221(90 * 8, airs_reader.getHDChannel(0).height(), 3);
            {
                image221.draw_image(0, airs_reader.getHDChannel(1));
                image221.draw_image(1, airs_reader.getHDChannel(1));
                image221.draw_image(2, airs_reader.getHDChannel(0));
            }
            WRITE_IMAGE(image221, directory + "/AIRS-HD-RGB-221.png");

            logger->info("HD 332 Composite...");
            image::Image<uint16_t> image332(90 * 8, airs_reader.getHDChannel(0).height(), 3);
            {
                image332.draw_image(0, airs_reader.getHDChannel(2));
                image332.draw_image(1, airs_reader.getHDChannel(2));
                image332.draw_image(2, airs_reader.getHDChannel(1));
            }
            WRITE_IMAGE(image332, directory + "/AIRS-HD-RGB-332.png");

            logger->info("HD 321 Composite...");
            image::Image<uint16_t> image321(90 * 8, airs_reader.getHDChannel(0).height(), 3);
            {
                image321.draw_image(0, airs_reader.getHDChannel(2));
                image321.draw_image(1, airs_reader.getHDChannel(1));
                image321.draw_image(2, airs_reader.getHDChannel(0));
            }
            WRITE_IMAGE(image321, directory + "/AIRS-HD-RGB-321.png");

            // Normal res projecition
            if (airs_reader.lines > 0)
            {
                int norad = EOS_AQUA_NORAD;

                // Setup Projecition
                std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_IFOV> proj_settings = std::make_shared<geodetic::projection::LEOScanProjectorSettings_IFOV>(
                    99,                                // Scan angle
                    1.1,                               // IFOV X scan angle
                    1.4,                               // IFOV Y scan angle
                    -0.5,                              // Roll offset
                    0.0,                               // Pitch offset
                    -2.5,                              // Yaw offset
                    -1.1,                              // Time offset
                    90,                                // Number of IFOVs
                    1,                                 // IFOV Width
                    1,                                 // IFOV Height
                    airs_reader.getChannel(0).width(), // Image width
                    true,                              // Invert scan
                    tle::getTLEfromNORAD(norad),       // TLEs
                    airs_reader.timestamps_ifov        // Timestamps
                );
                geodetic::projection::LEOScanProjector projector(proj_settings);

                {
                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                    geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/AIRS.georef");
                }

                logger->info("Projected 62 channel...");
                image::Image<uint8_t> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(airs_reader.getChannel(62), projector, 2048 * 4, 1024 * 4, 1);
                WRITE_IMAGE(projected_image, directory + "/AIRS-62-PROJ.png");
            }

            // HD Projection
            if (airs_reader.lines > 0)
            {
                int norad = EOS_AQUA_NORAD;

                // Setup Projecition
                std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_IFOV> proj_settings = std::make_shared<geodetic::projection::LEOScanProjectorSettings_IFOV>(
                    99.0,                                // Scan angle
                    1.1,                                 // IFOV X scan angle
                    1.4,                                 // IFOV Y scan angle
                    -0.5,                                // Roll offset
                    0.0,                                 // Pitch offset
                    -2.7,                                // Yaw offset
                    -1.1,                                // Time offset
                    90,                                  // Number of IFOVs
                    8,                                   // IFOV Width
                    9,                                   // IFOV Height
                    airs_reader.getHDChannel(0).width(), // Image width
                    true,                                // Invert scan
                    tle::getTLEfromNORAD(norad),         // TLEs
                    airs_reader.timestamps_ifov          // Timestamps
                );
                geodetic::projection::LEOScanProjector projector(proj_settings);

                {
                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                    geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/AIRS-HD.georef");
                }

                logger->info("Projected HD RGB 321 channel...");
                image::Image<uint8_t> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image321, projector, 2048 * 4, 1024 * 4, 3);
                WRITE_IMAGE(projected_image, directory + "/AIRS-HD-RGB-321-PROJ.png");
            }
        }

        void AquaAIRSDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Aqua AIRS Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string AquaAIRSDecoderModule::getID()
        {
            return "aqua_airs";
        }

        std::vector<std::string> AquaAIRSDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> AquaAIRSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<AquaAIRSDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace airs
} // namespace aqua