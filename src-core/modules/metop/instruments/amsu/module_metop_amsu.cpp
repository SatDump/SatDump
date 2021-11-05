#include "module_metop_amsu.h"
#include <fstream>
#include "amsu_a1_reader.h"
#include "amsu_a2_reader.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "nlohmann/json_utils.h"
#include "common/geodetic/projection/satellite_reprojector.h"
#include "common/geodetic/projection/proj_file.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace metop
{
    namespace amsu
    {
        MetOpAMSUDecoderModule::MetOpAMSUDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void MetOpAMSUDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AMSU";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer(882, true);
            AMSUA1Reader a1reader;
            AMSUA2Reader a2reader;
            uint64_t amsu_cadu = 0, ccsds = 0, amsu1_ccsds = 0, amsu2_ccsds = 0;
            uint8_t cadu[1024];

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 3 is AMSU)
                if (vcdu.vcid == 3)
                {
                    amsu_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 64)
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 39)
                        {
                            a1reader.work(pkt);
                            amsu1_ccsds++;
                        }
                        if (pkt.header.apid == 40)
                        {
                            a2reader.work(pkt);
                            amsu2_ccsds++;
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

            logger->info("VCID 3 (AMSU) Frames : " + std::to_string(amsu_cadu));
            logger->info("CCSDS Frames         : " + std::to_string(ccsds));
            logger->info("AMSU A1 Frames       : " + std::to_string(amsu1_ccsds));
            logger->info("AMSU A2 Frames       : " + std::to_string(amsu2_ccsds));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            for (int i = 0; i < 2; i++)
            {
                logger->info("Channel " + std::to_string(i + 1) + "...");
                WRITE_IMAGE(a2reader.getChannel(i), directory + "/AMSU-A2-" + std::to_string(i + 1) + ".png");
                WRITE_IMAGE(a2reader.getChannel(i).equalize(1000).normalize(0, 65535), directory + "/AMSU-A2-" + std::to_string(i + 1) + "-EQU.png");
            }

            for (int i = 0; i < 13; i++)
            {
                logger->info("Channel " + std::to_string(i + 3) + "...");
                WRITE_IMAGE(a1reader.getChannel(i), directory + "/AMSU-A1-" + std::to_string(i + 3) + ".png");
                WRITE_IMAGE(a1reader.getChannel(i).equalize(1000).normalize(0, 65535), directory + "/AMSU-A1-" + std::to_string(i + 3) + "-EQU.png");
            }

            // Output a few nice composites as well
            logger->info("Global Composite...");
            cimg_library::CImg<unsigned short> imageAll(30 * 8, a1reader.getChannel(0).height() * 2, 1, 1, 0);
            {
                int height = a1reader.getChannel(0).height();

                // Row 1
                imageAll.draw_image(30 * 0, 0, 0, 0, a2reader.getChannel(0));
                imageAll.draw_image(30 * 1, 0, 0, 0, a2reader.getChannel(1));
                imageAll.draw_image(30 * 2, 0, 0, 0, a1reader.getChannel(0));
                imageAll.draw_image(30 * 3, 0, 0, 0, a1reader.getChannel(1));
                imageAll.draw_image(30 * 4, 0, 0, 0, a1reader.getChannel(2));
                imageAll.draw_image(30 * 5, 0, 0, 0, a1reader.getChannel(3));
                imageAll.draw_image(30 * 6, 0, 0, 0, a1reader.getChannel(4));
                imageAll.draw_image(30 * 7, 0, 0, 0, a1reader.getChannel(5));

                // Row 2
                imageAll.draw_image(30 * 0, height, 0, 0, a1reader.getChannel(6));
                imageAll.draw_image(30 * 1, height, 0, 0, a1reader.getChannel(7));
                imageAll.draw_image(30 * 2, height, 0, 0, a1reader.getChannel(8));
                imageAll.draw_image(30 * 3, height, 0, 0, a1reader.getChannel(9));
                imageAll.draw_image(30 * 4, height, 0, 0, a1reader.getChannel(10));
                imageAll.draw_image(30 * 5, height, 0, 0, a1reader.getChannel(11));
                imageAll.draw_image(30 * 6, height, 0, 0, a1reader.getChannel(12));
            }
            WRITE_IMAGE(imageAll, directory + "/AMSU-ALL.png");
            imageAll = cimg_library::CImg<unsigned short>(30 * 8, a1reader.getChannel(0).height() * 2, 1, 1, 0);
            {
                int height = a1reader.getChannel(0).height();

                // Row 1
                imageAll.draw_image(30 * 0, 0, 0, 0, a2reader.getChannel(0).equalize(1000).normalize(0, 65535));
                imageAll.draw_image(30 * 1, 0, 0, 0, a2reader.getChannel(1).equalize(1000).normalize(0, 65535));
                imageAll.draw_image(30 * 2, 0, 0, 0, a1reader.getChannel(0).equalize(1000).normalize(0, 65535));
                imageAll.draw_image(30 * 3, 0, 0, 0, a1reader.getChannel(1).equalize(1000).normalize(0, 65535));
                imageAll.draw_image(30 * 4, 0, 0, 0, a1reader.getChannel(2).equalize(1000).normalize(0, 65535));
                imageAll.draw_image(30 * 5, 0, 0, 0, a1reader.getChannel(3).equalize(1000).normalize(0, 65535));
                imageAll.draw_image(30 * 6, 0, 0, 0, a1reader.getChannel(4).equalize(1000).normalize(0, 65535));
                imageAll.draw_image(30 * 7, 0, 0, 0, a1reader.getChannel(5).equalize(1000).normalize(0, 65535));

                // Row 2
                imageAll.draw_image(30 * 0, height, 0, 0, a1reader.getChannel(6).equalize(1000).normalize(0, 65535));
                imageAll.draw_image(30 * 1, height, 0, 0, a1reader.getChannel(7).equalize(1000).normalize(0, 65535));
                imageAll.draw_image(30 * 2, height, 0, 0, a1reader.getChannel(8).equalize(1000).normalize(0, 65535));
                imageAll.draw_image(30 * 3, height, 0, 0, a1reader.getChannel(9).equalize(1000).normalize(0, 65535));
                imageAll.draw_image(30 * 4, height, 0, 0, a1reader.getChannel(10).equalize(1000).normalize(0, 65535));
                imageAll.draw_image(30 * 5, height, 0, 0, a1reader.getChannel(11).equalize(1000).normalize(0, 65535));
                imageAll.draw_image(30 * 6, height, 0, 0, a1reader.getChannel(12).equalize(1000).normalize(0, 65535));
            }
            WRITE_IMAGE(imageAll, directory + "/AMSU-ALL-EQU.png");

            // Reproject to an equirectangular proj
            if (a1reader.lines > 0 && a2reader.lines > 0)
            {
                // Get satellite info
                nlohmann::json satData = loadJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json");
                int norad = satData.contains("norad") > 0 ? satData["norad"].get<int>() : 0;

                // Setup Projecition. Twice with the same parameters except we need different timestamps
                // There is no "real" guarantee the A1 / A2 output will always be identical
                // Using the "/ 40" in instrument res slows things down but also avoids huge gaps
                // in the resulting image...
                std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_SCANLINE> proj_settings_a1 = std::make_shared<geodetic::projection::LEOScanProjectorSettings_SCANLINE>(
                    98,                             // Scan angle
                    0,                              // Roll offset
                    0,                              // Pitch offset
                    0,                              // Yaw offset
                    10,                             // Time offset
                    a1reader.getChannel(0).width(), // Image width
                    true,                           // Invert scan
                    tle::getTLEfromNORAD(norad),    // TLEs
                    a1reader.timestamps             // Timestamps
                );
                std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_SCANLINE> proj_settings_a2 = std::make_shared<geodetic::projection::LEOScanProjectorSettings_SCANLINE>(
                    98,                             // Scan angle
                    0,                              // Roll offset
                    0,                              // Pitch offset
                    0,                              // Yaw offset
                    10,                             // Time offset
                    a2reader.getChannel(0).width(), // Image width
                    true,                           // Invert scan
                    tle::getTLEfromNORAD(norad),    // TLEs
                    a2reader.timestamps             // Timestamps
                );
                geodetic::projection::LEOScanProjector projector_a1(proj_settings_a1);
                geodetic::projection::LEOScanProjector projector_a2(proj_settings_a2);

                {
                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile_a1 = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings_a1);
                    geodetic::projection::proj_file::writeReferenceFile(geofile_a1, directory + "/AMSU-A1.georef");
                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile_a2 = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings_a2);
                    geodetic::projection::proj_file::writeReferenceFile(geofile_a2, directory + "/AMSU-A2.georef");
                }

                for (int i = 0; i < 13; i++)
                {
                    cimg_library::CImg<unsigned short> image = a1reader.getChannel(i).equalize(1000).normalize(0, 65535);
                    image.equalize(1000);
                    image.normalize(0, 65535);
                    logger->info("Projected channel A1 " + std::to_string(i + 3) + "...");
                    cimg_library::CImg<unsigned char> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image, projector_a1, 1024, 512);
                    WRITE_IMAGE(projected_image, directory + "/AMSU-A1-" + std::to_string(i + 3) + "-PROJ.png");
                }

                for (int i = 0; i < 2; i++)
                {
                    cimg_library::CImg<unsigned short> image = a2reader.getChannel(i).equalize(1000).normalize(0, 65535);
                    image.equalize(1000);
                    image.normalize(0, 65535);
                    logger->info("Projected channel A2 " + std::to_string(i + 1) + "...");
                    cimg_library::CImg<unsigned char> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image, projector_a2, 1024, 512);
                    WRITE_IMAGE(projected_image, directory + "/AMSU-A2-" + std::to_string(i + 1) + "-PROJ.png");
                }
            }
        }

        void MetOpAMSUDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("MetOp AMSU Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string MetOpAMSUDecoderModule::getID()
        {
            return "metop_amsu";
        }

        std::vector<std::string> MetOpAMSUDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> MetOpAMSUDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<MetOpAMSUDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace metop