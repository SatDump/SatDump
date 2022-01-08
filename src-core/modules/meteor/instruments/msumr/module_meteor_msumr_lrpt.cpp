#include "module_meteor_msumr_lrpt.h"
#include <fstream>
#include "modules/meteor/simpledeframer.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include <cstring>
#include "lrpt_msumr_reader.h"
#include "imgui/imgui.h"
#include "common/image/earth_curvature.h"
#include "modules/meteor/meteor.h"
#include "common/geodetic/projection/satellite_reprojector.h"
#include "common/geodetic/projection/proj_file.h"
#include "common/image/composite.h"
#include "common/map/leo_drawer.h"
#include <ctime>

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace meteor
{
    namespace msumr
    {
        METEORMSUMRLRPTDecoderModule::METEORMSUMRLRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void METEORMSUMRLRPTDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MSU-MR";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            uint8_t cadu[1024];

            ccsds::ccsds_1_0_1024::Demuxer ccsds_demuxer(882, true);

            lrpt::MSUMRReader msureader;

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                data_in.read((char *)&cadu, 1024);

                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                if (vcdu.vcid == 5)
                {
                    std::vector<ccsds::CCSDSPacket> frames = ccsds_demuxer.work(cadu);
                    for (ccsds::CCSDSPacket pkt : frames)
                    {
                        msureader.work(pkt);
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

            logger->info("MSU-MR Channel 1 Lines  : " + std::to_string(msureader.getChannel(0).height()));
            logger->info("MSU-MR Channel 2 Lines  : " + std::to_string(msureader.getChannel(1).height()));
            logger->info("MSU-MR Channel 3 Lines  : " + std::to_string(msureader.getChannel(2).height()));
            logger->info("MSU-MR Channel 4 Lines  : " + std::to_string(msureader.getChannel(3).height()));
            logger->info("MSU-MR Channel 5 Lines  : " + std::to_string(msureader.getChannel(4).height()));
            logger->info("MSU-MR Channel 6 Lines  : " + std::to_string(msureader.getChannel(5).height()));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            // TODO : Add detection system!!!!!!!!
            // Currently not *that* mandatory as only M2 is active on VHF
            int norad = 40069;

            // Setup Projecition, tuned for M2
            std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_SCANLINE> proj_settings = geodetic::projection::makeScalineSettingsFromJSON("meteor_m2_msumr_lrpt.json");
            proj_settings->sat_tle = tle::getTLEfromNORAD(norad);

            for (int channel = 0; channel < 6; channel++)
            {
                logger->info("Channel " + std::to_string(channel + 1) + "...");
                if (msureader.getChannel(channel).height() > 0)
                {
                    WRITE_IMAGE(msureader.getChannel(channel), directory + "/MSU-MR-" + std::to_string(channel + 1) + ".png");
                    proj_settings->utc_timestamps = msureader.timestamps;
                    geodetic::projection::LEOScanProjector projector(proj_settings);
                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                    geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/MSU-MR-" + std::to_string(channel + 1) + ".georef");
                }
                else
                {
                    logger->info("Channel empty, skipped.");
                }
            }

            // Generate composites
            for (const nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> &compokey : d_parameters["composites"].items())
            {
                nlohmann::json compositeDef = compokey.value();

                std::string expression = compositeDef["expression"].get<std::string>();
                bool corrected = compositeDef.count("corrected") > 0 ? compositeDef["corrected"].get<bool>() : false;
                bool mapped = compositeDef.count("mapped") > 0 ? compositeDef["mapped"].get<bool>() : false;
                bool projected = compositeDef.count("projected") > 0 ? compositeDef["projected"].get<bool>() : false;

                std::string name = "MSU-MR-" + compokey.key();

                // Get required channels
                std::vector<int> requiredChannels = compositeDef["channels"].get<std::vector<int>>();

                if (requiredChannels.size() > 3)
                {
                    logger->info("Maximum number of channels must be 1, 2 or 3!");
                    continue;
                }

                // Check they are all present
                bool foundAll = true;
                for (int channel : requiredChannels)
                {
                    if (msureader.lines[channel - 1] > 0)
                    {
                        logger->debug("Channel " + std::to_string(channel) + " is present!");
                    }
                    else
                    {
                        foundAll = false;
                        break;
                    }
                }

                if (!foundAll)
                {
                    logger->error("Some channels are missing, skipping composite.");
                    continue;
                }

                // Correlate channels
                std::vector<image::Image<uint8_t>> channels;
                std::vector<int> channel_numbers;
                if (requiredChannels.size() == 1)
                {
                    channels.push_back(msureader.getChannel(requiredChannels[0] - 1));
                    channel_numbers.push_back(requiredChannels[0]);
                }
                else if (requiredChannels.size() == 2)
                {
                    std::array<int32_t, 3> correlated = msureader.correlateChannels(requiredChannels[0] - 1, requiredChannels[1] - 1);

                    channels.push_back(msureader.getChannel(requiredChannels[0] - 1, correlated[0], correlated[1], correlated[2]));
                    channel_numbers.push_back(requiredChannels[0]);
                    channels.push_back(msureader.getChannel(requiredChannels[1] - 1, correlated[0], correlated[1], correlated[2]));
                    channel_numbers.push_back(requiredChannels[1]);
                }
                else if (requiredChannels.size() == 3)
                {
                    std::array<int32_t, 3> correlated = msureader.correlateChannels(requiredChannels[0] - 1, requiredChannels[1] - 1, requiredChannels[2] - 1);

                    channels.push_back(msureader.getChannel(requiredChannels[0] - 1, correlated[0], correlated[1], correlated[2]));
                    channel_numbers.push_back(requiredChannels[0]);
                    channels.push_back(msureader.getChannel(requiredChannels[1] - 1, correlated[0], correlated[1], correlated[2]));
                    channel_numbers.push_back(requiredChannels[1]);
                    channels.push_back(msureader.getChannel(requiredChannels[2] - 1, correlated[0], correlated[1], correlated[2]));
                    channel_numbers.push_back(requiredChannels[2]);
                }

                // Generate GEOREF
                proj_settings->utc_timestamps = msureader.timestamps;
                geodetic::projection::LEOScanProjector projector(proj_settings);
                geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/" + name + ".georef");

                logger->info(name + "...");
                image::Image<uint8_t> compositeImage = image::generate_composite_from_equ<unsigned char>(channels,
                                                                                                         channel_numbers,
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
                    projector.setup_forward(99, 1, 16);
                    logger->info(name + "-MAP...");
                    image::Image<uint8_t> mapped_image = map::drawMapToLEO(compositeImage, projector);
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

        void METEORMSUMRLRPTDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("METEOR MSU-MR LRPT Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string METEORMSUMRLRPTDecoderModule::getID()
        {
            return "meteor_msumr_lrpt";
        }

        std::vector<std::string> METEORMSUMRLRPTDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> METEORMSUMRLRPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<METEORMSUMRLRPTDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace msumr
} // namespace meteor
