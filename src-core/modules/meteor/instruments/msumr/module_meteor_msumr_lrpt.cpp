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
#include "common/image/image.h"
#include "modules/meteor/meteor.h"
#include "common/geodetic/projection/satellite_reprojector.h"
#include "common/geodetic/projection/proj_file.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace meteor
{
    namespace msumr
    {
        METEORMSUMRLRPTDecoderModule::METEORMSUMRLRPTDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
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
            std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_SCANLINE> proj_settings = std::make_shared<geodetic::projection::LEOScanProjectorSettings_SCANLINE>(
                110.6,                       // Scan angle
                2.3,                         // Roll offset
                0,                           // Pitch offset
                -2.8,                        // Yaw offset
                1.4,                         // Time offset
                1568,                        // Image width
                true,                        // Invert scan
                tle::getTLEfromNORAD(norad), // TLEs
                std::vector<double>()        // Timestamps
            );

            for (int channel = 0; channel < 6; channel++)
            {
                logger->info("Channel " + std::to_string(channel + 1) + "...");
                if (msureader.getChannel(channel).size() > 0)
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

            if (msureader.getChannel(0).size() > 0 && msureader.getChannel(1).size() > 0)
            {
                std::array<int32_t, 3> correlated = msureader.correlateChannels(0, 1);

                cimg_library::CImg<unsigned short> image1 = msureader.getChannel(0, correlated[0], correlated[1], correlated[2]);
                cimg_library::CImg<unsigned short> image2 = msureader.getChannel(1, correlated[0], correlated[1], correlated[2]);

                logger->info("221 Composite...");
                cimg_library::CImg<unsigned short> image221(image1.width(), image2.height(), 1, 3);
                {
                    image221.draw_image(0, 0, 0, 0, image2);
                    image221.draw_image(0, 0, 0, 1, image2);
                    image221.draw_image(0, 0, 0, 2, image1);
                }
                WRITE_IMAGE(image221, directory + "/MSU-MR-RGB-221.png");
                cimg_library::CImg<unsigned short> corrected221 = image::earth_curvature::correct_earth_curvature(image221,
                                                                                                                  METEOR_ORBIT_HEIGHT,
                                                                                                                  METEOR_MSUMR_SWATH,
                                                                                                                  METEOR_MSUMR_RES);
                WRITE_IMAGE(corrected221, directory + "/MSU-MR-RGB-221-CORRECTED.png");
                image221.equalize(1000);
                image221.normalize(0, std::numeric_limits<unsigned char>::max());
                WRITE_IMAGE(image221, directory + "/MSU-MR-RGB-221-EQU.png");
                WRITE_IMAGE(image221, directory + "/MSU-MR-RGB-221-EQU.png");
                corrected221.equalize(1000);
                corrected221.normalize(0, std::numeric_limits<unsigned char>::max());
                WRITE_IMAGE(corrected221, directory + "/MSU-MR-RGB-221-EQU-CORRECTED.png");

                proj_settings->utc_timestamps = msureader.timestamps;
                geodetic::projection::LEOScanProjector projector(proj_settings);
                geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/MSU-MR-RGB-221.georef");
                logger->info("Projected Channel RGB 221...");
                cimg_library::CImg<unsigned char> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image221 << 8, projector, 2048 * 4, 1024 * 4, 3);
                WRITE_IMAGE(projected_image, directory + "/MSU-MR-RGB-221-PROJ.png");
            }

            if (msureader.getChannel(0).size() > 0 && msureader.getChannel(1).size() > 0 && msureader.getChannel(2).size() > 0)
            {
                std::array<int32_t, 3> correlated = msureader.correlateChannels(0, 1, 2);

                cimg_library::CImg<unsigned short> image1 = msureader.getChannel(0, correlated[0], correlated[1], correlated[2]);
                cimg_library::CImg<unsigned short> image2 = msureader.getChannel(1, correlated[0], correlated[1], correlated[2]);
                cimg_library::CImg<unsigned short> image3 = msureader.getChannel(2, correlated[0], correlated[1], correlated[2]);

                // Check if channel 3 is empty and proceed if it has data
                logger->info("321 Composite...");
                cimg_library::CImg<unsigned short> image321(image1.width(), image2.height(), 1, 3);
                {
                    image321.draw_image(0, 0, 0, 0, image3);
                    image321.draw_image(0, 0, 0, 1, image2);
                    image321.draw_image(0, 0, 0, 2, image1);
                }
                WRITE_IMAGE(image321, directory + "/MSU-MR-RGB-321.png");
                cimg_library::CImg<unsigned short> corrected321 = image::earth_curvature::correct_earth_curvature(image321,
                                                                                                                  METEOR_ORBIT_HEIGHT,
                                                                                                                  METEOR_MSUMR_SWATH,
                                                                                                                  METEOR_MSUMR_RES);
                WRITE_IMAGE(corrected321, directory + "/MSU-MR-RGB-321-CORRECTED.png");
                image321.equalize(1000);
                image321.normalize(0, std::numeric_limits<unsigned char>::max());
                WRITE_IMAGE(image321, directory + "/MSU-MR-RGB-321-EQU.png");
                WRITE_IMAGE(image321, directory + "/MSU-MR-RGB-321-EQU.png");
                corrected321.equalize(1000);
                corrected321.normalize(0, std::numeric_limits<unsigned char>::max());
                WRITE_IMAGE(corrected321, directory + "/MSU-MR-RGB-321-EQU-CORRECTED.png");

                proj_settings->utc_timestamps = msureader.timestamps;
                geodetic::projection::LEOScanProjector projector(proj_settings);
                geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/MSU-MR-RGB-321.georef");
            }

            if (msureader.getChannel(4).size() > 0)
            {
                cimg_library::CImg<unsigned short> image5 = msureader.getChannel(4);
                logger->info("Equalized Ch 5...");
                image::linear_invert(image5);
                image5.equalize(1000);
                image5.normalize(0, std::numeric_limits<unsigned char>::max());
                WRITE_IMAGE(image5, directory + "/MSU-MR-5-EQU.png");
                cimg_library::CImg<unsigned short> corrected5 = image::earth_curvature::correct_earth_curvature(image5,
                                                                                                                METEOR_ORBIT_HEIGHT,
                                                                                                                METEOR_MSUMR_SWATH,
                                                                                                                METEOR_MSUMR_RES);
                WRITE_IMAGE(corrected5, directory + "/MSU-MR-5-EQU-CORRECTED.png");

                proj_settings->utc_timestamps = msureader.timestamps;
                geodetic::projection::LEOScanProjector projector(proj_settings);
                logger->info("Projected Channel 5...");
                cimg_library::CImg<unsigned char> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image5 << 8, projector, 2048 * 4, 1024 * 4, 1);
                WRITE_IMAGE(projected_image, directory + "/MSU-MR-5-PROJ.png");
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

        std::shared_ptr<ProcessingModule> METEORMSUMRLRPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<METEORMSUMRLRPTDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace msumr
} // namespace meteor
