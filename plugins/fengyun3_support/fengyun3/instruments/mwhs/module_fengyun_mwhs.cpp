#define cimg_use_jpeg
#include "module_fengyun_mwhs.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "mwhs_reader.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "nlohmann/json_utils.h"
#include "common/geodetic/projection/satellite_reprojector.h"
#include "common/geodetic/projection/proj_file.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun3
{
    namespace mwhs
    {
        FengyunMWHSDecoderModule::FengyunMWHSDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void FengyunMWHSDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWHS";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            int vcidFrames = 0, ccsds_frames = 0;

            // Read buffer
            uint8_t buffer[1024];

            logger->info("Demultiplexing and deframing... (WIP!)");

            // Demuxer
            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer = ccsds::ccsds_1_0_1024::Demuxer(882, true);

            // Reader
            MWHSReader mwhs_reader;

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
                        if (pkt.header.apid == 16)
                            mwhs_reader.work(pkt);
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
            logger->info("MWHS Lines             : " + std::to_string(mwhs_reader.lines));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            for (int i = 0; i < 6; i++)
            {
                logger->info("Channel " + std::to_string(i + 1) + "...");
                WRITE_IMAGE(mwhs_reader.getChannel(i), directory + "/MWHS-" + std::to_string(i + 1) + ".png");
            }

            // Output a few nice composites as well
            logger->info("Global Composite...");
            image::Image<uint16_t> imageAll(98 * 3, mwhs_reader.getChannel(0).height() * 2, 1);
            {
                int height = mwhs_reader.getChannel(0).height();

                // Row 1
                imageAll.draw_image(0, mwhs_reader.getChannel(0), 98 * 0, 0);
                imageAll.draw_image(0, mwhs_reader.getChannel(1), 98 * 1, 0);
                imageAll.draw_image(0, mwhs_reader.getChannel(2), 98 * 2, 0);

                // Row 2
                imageAll.draw_image(0, mwhs_reader.getChannel(3), 98 * 0, height);
                imageAll.draw_image(0, mwhs_reader.getChannel(4), 98 * 1, height);
                imageAll.draw_image(0, mwhs_reader.getChannel(5), 98 * 2, height);
            }
            WRITE_IMAGE(imageAll, directory + "/MWHS-ALL.png");

            // Reproject to an equirectangular proj
            if (mwhs_reader.lines > 0 && mwhs_reader.timestamps.size() > 0)
            {
                // Get satellite info
                nlohmann::json satData = loadJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json");
                int norad = satData.contains("norad") > 0 ? satData["norad"].get<int>() : 0;

                // Setup Projecition
                std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_SCANLINE> proj_settings = geodetic::projection::makeScalineSettingsFromJSON("fengyun_ab_mwhs1.json");
                proj_settings->sat_tle = tle::getTLEfromNORAD(norad);   // TLEs
                proj_settings->utc_timestamps = mwhs_reader.timestamps; // Timestamps
                geodetic::projection::LEOScanProjector projector(proj_settings);

                {
                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                    geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/MWHS.georef");
                }

                for (int i = 0; i < 5; i++)
                {
                    image::Image<uint16_t> image = mwhs_reader.getChannel(i);
                    logger->info("Projected Channel " + std::to_string(i + 1) + "...");
                    image::Image<uint8_t> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image, projector, 2048, 1024);
                    WRITE_IMAGE(projected_image, directory + "/MWHS-" + std::to_string(i + 1) + "-PROJ.png");
                }
            }
        }

        void FengyunMWHSDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("FengYun MWHS Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string FengyunMWHSDecoderModule::getID()
        {
            return "fengyun_mwhs";
        }

        std::vector<std::string> FengyunMWHSDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> FengyunMWHSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<FengyunMWHSDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace virr
} // namespace fengyun