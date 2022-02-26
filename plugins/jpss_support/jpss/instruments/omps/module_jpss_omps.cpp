#include "module_jpss_omps.h"
#include <fstream>
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "omps_nadir_reader.h"
#include "omps_limb_reader.h"
#include "nlohmann/json_utils.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace jpss
{
    namespace omps
    {
        JPSSOMPSDecoderModule::JPSSOMPSDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
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

        void JPSSOMPSDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/OMPS";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            logger->info("Demultiplexing and deframing...");

            // Read buffer
            uint8_t cadu[1024];

            // Counters
            uint64_t omps_cadu = 0, ccsds = 0, omps_ccsds = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer = ccsds::ccsds_1_0_1024::Demuxer(mpdu_size, false);

            omps::OMPSNadirReader omps_nadir_reader;
            omps::OMPSLimbReader omps_limb_reader;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, cadu_size);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 11 is OMPS)
                if (vcdu.vcid == 11)
                {
                    omps_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 616 || pkt.header.apid == 560)
                        {
                            omps_nadir_reader.work(pkt);
                        }

                        if (pkt.header.apid == 617 || pkt.header.apid == 561)
                        {
                            omps_limb_reader.work(pkt);
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

            logger->info("VCID 11 (OMPS) Frames  : " + std::to_string(omps_cadu));
            logger->info("CCSDS Frames           : " + std::to_string(ccsds));
            logger->info("OMPS CCSDS Frames      : " + std::to_string(omps_ccsds));
            logger->info("OMPS Nadir Lines       : " + std::to_string(omps_nadir_reader.lines));
            logger->info("OMPS Limb Lines        : " + std::to_string(omps_limb_reader.lines));

            logger->info("Writing images.... (Can take a while)");

            for (int i = 0; i < 339; i++)
            {
                logger->info("Channel " + std::to_string(i + 1) + "...");
                WRITE_IMAGE(omps_nadir_reader.getChannel(i), directory + "/OMPS-NADIR-" + std::to_string(i + 1) + ".png");
            }

            for (int i = 0; i < 135; i++)
            {
                logger->info("Channel " + std::to_string(i + 1) + "...");
                WRITE_IMAGE(omps_limb_reader.getChannel(i), directory + "/OMPS-LIMB-" + std::to_string(i + 1) + ".png");
            }

            /*if (omps_nadir_reader.lines > 0)
            {
                // Get satellite info
                nlohmann::json satData = loadJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json");
                int norad = satData.contains("norad") > 0 ? satData["norad"].get<int>() : 0;

                cimg_library::CImg<unsigned short> image86 = omps_nadir_reader.getChannel(85);

                // Setup Projecition
                geodetic::projection::LEOScanProjectorSettings proj_settings = {
                    102,                         // Scan angle
                    -0.5,                        // Roll offset
                    0,                           // Pitch offset
                    -3,                          // Yaw offset
                    2,                           // Time offset
                    image86.width(),             // Image width
                    true,                        // Invert scan
                    tle::getTLEfromNORAD(norad), // TLEs
                    omps_nadir_reader.timestamps // Timestamps
                };
                geodetic::projection::LEOScanProjector projector(proj_settings);

                {
                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                    geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/OMPS.georef");
                }

                image86.equalize(1000);
                logger->info("Projected channel 86...");
                cimg_library::CImg<unsigned char> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image86, projector, 2048, 1024);
                WRITE_IMAGE(projected_image, directory + "/OMPS-NADIR-86-PROJ.png");
            }*/
        }

        void JPSSOMPSDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("JPSS OMPS Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string JPSSOMPSDecoderModule::getID()
        {
            return "jpss_omps";
        }

        std::vector<std::string> JPSSOMPSDecoderModule::getParameters()
        {
            return {"npp_mode"};
        }

        std::shared_ptr<ProcessingModule> JPSSOMPSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<JPSSOMPSDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace atms
} // namespace jpss