#include "module_aqua_amsu.h"
#include <fstream>
#include "amsu_a1_reader.h"
#include "amsu_a2_reader.h"
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
    namespace amsu
    {
        AquaAMSUDecoderModule::AquaAMSUDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void AquaAMSUDecoderModule::process()
        {
            size_t filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AMSU";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            // Read buffer
            uint8_t cadu[1024];

            // Counters
            uint64_t amsu_cadu = 0, ccsds = 0, amsu1_ccsds = 0, amsu2_ccsds = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer1 = ccsds::ccsds_1_0_1024::Demuxer();
            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer2 = ccsds::ccsds_1_0_1024::Demuxer();

            // Readers
            AMSUA1Reader a1reader;
            AMSUA2Reader a2reader;

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 20 / 25 is AMSU)
                if (vcdu.vcid == 20)
                {
                    amsu_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer1.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 64)
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 261 || pkt.header.apid == 262)
                        {
                            a1reader.work(pkt);
                            amsu1_ccsds++;
                        }
                    }
                }

                if (vcdu.vcid == 25)
                {
                    amsu_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer2.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 64)
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 290)
                        {
                            a2reader.work(pkt);
                            amsu2_ccsds++;
                        }
                    }
                }

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)data_in.tellg() / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            logger->info("VCID 20/25 (AMSU) Frames : " + std::to_string(amsu_cadu));
            logger->info("CCSDS Frames             : " + std::to_string(ccsds));
            logger->info("AMSU A1 Frames           : " + std::to_string(amsu1_ccsds));
            logger->info("AMSU A2 Frames           : " + std::to_string(amsu2_ccsds));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            for (int i = 0; i < 2; i++)
            {
                logger->info("Channel " + std::to_string(i + 1) + "...");
                WRITE_IMAGE(a2reader.getChannel(i), directory + "/AMSU-A2-" + std::to_string(i + 1) + ".png");
            }

            for (int i = 0; i < 13; i++)
            {
                logger->info("Channel " + std::to_string(i + 3) + "...");
                WRITE_IMAGE(a1reader.getChannel(i), directory + "/AMSU-A1-" + std::to_string(i + 3) + ".png");
            }

            // Output a few nice composites as well
            logger->info("Global Composite...");
            image::Image<uint16_t> imageAll(30 * 8, a1reader.getChannel(0).height() * 2, 1);
            {
                int height = a1reader.getChannel(0).height();

                // Row 1
                imageAll.draw_image(0, a2reader.getChannel(0), 30 * 0);
                imageAll.draw_image(0, a2reader.getChannel(1), 30 * 1);
                imageAll.draw_image(0, a1reader.getChannel(0), 30 * 2);
                imageAll.draw_image(0, a1reader.getChannel(1), 30 * 3);
                imageAll.draw_image(0, a1reader.getChannel(2), 30 * 4);
                imageAll.draw_image(0, a1reader.getChannel(3), 30 * 5);
                imageAll.draw_image(0, a1reader.getChannel(4), 30 * 6);
                imageAll.draw_image(0, a1reader.getChannel(5), 30 * 7);

                // Row 2
                imageAll.draw_image(0, a1reader.getChannel(6), 30 * 0, height);
                imageAll.draw_image(0, a1reader.getChannel(7), 30 * 1, height);
                imageAll.draw_image(0, a1reader.getChannel(8), 30 * 2, height);
                imageAll.draw_image(0, a1reader.getChannel(9), 30 * 3, height);
                imageAll.draw_image(0, a1reader.getChannel(10), 30 * 4, height);
                imageAll.draw_image(0, a1reader.getChannel(11), 30 * 5, height);
                imageAll.draw_image(0, a1reader.getChannel(12), 30 * 6, height);
            }
            WRITE_IMAGE(imageAll, directory + "/AMSU-ALL.png");

            // Reproject to an equirectangular proj
            if (a1reader.lines > 0 || a2reader.lines > 0)
            {
                // Get satellite info
                int norad = EOS_AQUA_NORAD;

                // Setup Projecition. Twice with the same parameters except we need different timestamps
                // There is no "real" guarantee the A1 / A2 output will always be identical
                std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_SCANLINE> proj_settings_a1 = geodetic::projection::makeScalineSettingsFromJSON("aqua_amsu.json");
                proj_settings_a1->sat_tle = tle::getTLEfromNORAD(norad); // TLEs
                proj_settings_a1->utc_timestamps = a1reader.timestamps;  // Timestamps
                std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_SCANLINE> proj_settings_a2 = geodetic::projection::makeScalineSettingsFromJSON("aqua_amsu.json");
                proj_settings_a2->sat_tle = tle::getTLEfromNORAD(norad); // TLEs
                proj_settings_a2->utc_timestamps = a2reader.timestamps;  // Timestamps

                if (a1reader.lines > 0)
                {
                    geodetic::projection::LEOScanProjector projector_a1(proj_settings_a1);

                    {
                        geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile_a1 = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings_a1);
                        geodetic::projection::proj_file::writeReferenceFile(geofile_a1, directory + "/AMSU-A1.georef");
                    }

                    for (int i = 0; i < 13; i++)
                    {
                        image::Image<uint16_t> image = a1reader.getChannel(i);
                        image.equalize();
                        image.normalize();
                        logger->info("Projected channel A1 " + std::to_string(i + 3) + "...");
                        image::Image<uint8_t> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image, projector_a1, 1024, 512);
                        WRITE_IMAGE(projected_image, directory + "/AMSU-A1-" + std::to_string(i + 3) + "-PROJ.png");
                    }
                }

                if (a2reader.lines > 0)
                {
                    geodetic::projection::LEOScanProjector projector_a2(proj_settings_a2);

                    {
                        geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile_a1 = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings_a1);
                        geodetic::projection::proj_file::writeReferenceFile(geofile_a1, directory + "/AMSU-A1.georef");
                        geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile_a2 = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings_a2);
                        geodetic::projection::proj_file::writeReferenceFile(geofile_a2, directory + "/AMSU-A2.georef");
                    }

                    for (int i = 0; i < 2; i++)
                    {
                        image::Image<uint16_t> image = a2reader.getChannel(i);
                        image.equalize();
                        image.normalize();
                        logger->info("Projected channel A2 " + std::to_string(i + 1) + "...");
                        image::Image<uint8_t> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image, projector_a2, 1024, 512);
                        WRITE_IMAGE(projected_image, directory + "/AMSU-A2-" + std::to_string(i + 1) + "-PROJ.png");
                    }
                }
            }
        }

        void AquaAMSUDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Aqua AMSU Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string AquaAMSUDecoderModule::getID()
        {
            return "aqua_amsu";
        }

        std::vector<std::string> AquaAMSUDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> AquaAMSUDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<AquaAMSUDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace aqua