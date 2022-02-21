#include "module_fengyun_mwts3.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "mwts3_reader.h"
#include "nlohmann/json_utils.h"
#include "common/geodetic/projection/satellite_reprojector.h"
#include "common/geodetic/projection/proj_file.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun3
{
    namespace mwts3
    {
        FengyunMWTS3DecoderModule::FengyunMWTS3DecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void FengyunMWTS3DecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWTS-3";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            int vcidFrames = 0, ccsds_frames = 0;

            // Read buffer
            uint8_t buffer[1024];

            logger->info("Demultiplexing and deframing...");

            // Demuxer
            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer = ccsds::ccsds_1_0_1024::Demuxer(882, true);

            // Reader
            MWTS3Reader mwts_reader;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 1024);

                // Extract VCID
                int vcid = buffer[5] % ((int)pow(2, 6));

                if (vcid == 12)
                {
                    vcidFrames++;

                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(buffer);
                    ccsds_frames += ccsdsFrames.size();

                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 7)
                        {
                            mwts_reader.work(pkt);
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

            logger->info("VCID 12 Frames         : " + std::to_string(vcidFrames));
            logger->info("CCSDS Frames           : " + std::to_string(ccsds_frames));
            logger->info("MWTS-3 Lines           : " + std::to_string(mwts_reader.lines));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            for (int i = 0; i < 18; i++)
            {
                logger->info("Channel " + std::to_string(i + 1) + "...");
                WRITE_IMAGE(mwts_reader.getChannel(i), directory + "/MWTS3-" + std::to_string(i + 1) + ".png");
            }

            // Output a few nice composites as well
            logger->info("Global Composite...");
            image::Image<uint16_t> imageAll(98 * 4, mwts_reader.getChannel(0).height() * 5, 1);
            {
                int height = mwts_reader.getChannel(0).height();

                // Row 1
                imageAll.draw_image(0, mwts_reader.getChannel(0), 98 * 0, 0);
                imageAll.draw_image(0, mwts_reader.getChannel(1), 98 * 1, 0);
                imageAll.draw_image(0, mwts_reader.getChannel(2), 98 * 2, 0);
                imageAll.draw_image(0, mwts_reader.getChannel(3), 98 * 3, 0);

                // Row 2
                imageAll.draw_image(0, mwts_reader.getChannel(4), 98 * 0, height);
                imageAll.draw_image(0, mwts_reader.getChannel(5), 98 * 1, height);
                imageAll.draw_image(0, mwts_reader.getChannel(6), 98 * 2, height);
                imageAll.draw_image(0, mwts_reader.getChannel(7), 98 * 3, height);

                // Row 2
                imageAll.draw_image(0, mwts_reader.getChannel(8), 98 * 0, height * 2);
                imageAll.draw_image(0, mwts_reader.getChannel(9), 98 * 1, height * 2);
                imageAll.draw_image(0, mwts_reader.getChannel(10), 98 * 2, height * 2);
                imageAll.draw_image(0, mwts_reader.getChannel(11), 98 * 3, height * 2);

                // Row 2
                imageAll.draw_image(0, mwts_reader.getChannel(12), 98 * 0, height * 3);
                imageAll.draw_image(0, mwts_reader.getChannel(13), 98 * 1, height * 3);
                imageAll.draw_image(0, mwts_reader.getChannel(14), 98 * 2, height * 3);
                imageAll.draw_image(0, mwts_reader.getChannel(15), 98 * 3, height * 3);

                // Row 2
                imageAll.draw_image(0, mwts_reader.getChannel(16), 98 * 0, height * 4);
                imageAll.draw_image(0, mwts_reader.getChannel(17), 98 * 1, height * 4);
            }
            WRITE_IMAGE(imageAll, directory + "/MWTS3-ALL.png");

            // Reproject to an equirectangular proj.
            // This instrument was a PAIN to align... So it's not perfect
            // Also the low sampling rate doesn't help
            if (mwts_reader.lines > 0)
            {
                // Get satellite info
                nlohmann::json satData = loadJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json");
                int norad = satData.contains("norad") > 0 ? satData["norad"].get<int>() : 0;

                // Setup Projecition
                std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_SCANLINE> proj_settings = geodetic::projection::makeScalineSettingsFromJSON("fengyun_e_mwts3.json");
                proj_settings->sat_tle = tle::getTLEfromNORAD(norad);   // TLEs
                proj_settings->utc_timestamps = mwts_reader.timestamps; // Timestamps
                geodetic::projection::LEOScanProjector projector(proj_settings);

                {
                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                    geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/MWTS-3.georef");
                }

                for (int i = 0; i < 18; i++)
                {
                    image::Image<uint16_t> image = mwts_reader.getChannel(i);
                    logger->info("Projected Channel " + std::to_string(i + 1) + "...");
                    image::Image<uint8_t> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image, projector, 2048 / 2, 1024 / 2);
                    WRITE_IMAGE(projected_image, directory + "/MWTS3-" + std::to_string(i + 1) + "-PROJ.png");
                }
            }
        }

        void FengyunMWTS3DecoderModule::drawUI(bool window)
        {
            ImGui::Begin("FengYun MWTS-3 Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string FengyunMWTS3DecoderModule::getID()
        {
            return "fengyun_mwts3";
        }

        std::vector<std::string> FengyunMWTS3DecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> FengyunMWTS3DecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<FengyunMWTS3DecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace virr
} // namespace fengyun