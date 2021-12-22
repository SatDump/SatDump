#include "module_metop_mhs.h"
#include <fstream>
#include "mhs_reader.h"
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
    namespace mhs
    {
        MetOpMHSDecoderModule::MetOpMHSDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void MetOpMHSDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MHS";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer(882, true);
            MHSReader mhsreader;
            uint64_t mhs_cadu = 0, ccsds = 0, mhs_ccsds = 0;
            uint8_t cadu[1024];

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 12 is MHS)
                if (vcdu.vcid == 12)
                {
                    mhs_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 64)
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 34)
                        {
                            mhsreader.work(pkt);
                            mhs_ccsds++;
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

            logger->info("VCID 12 (MHS) Frames : " + std::to_string(mhs_cadu));
            logger->info("CCSDS Frames         : " + std::to_string(ccsds));
            logger->info("MHS Frames           : " + std::to_string(mhs_ccsds));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            for (int i = 0; i < 5; i++)
            {
                logger->info("Channel " + std::to_string(i + 1) + "...");
                WRITE_IMAGE(mhsreader.getChannel(i), directory + "/MHS-" + std::to_string(i + 1) + ".png");
                image::Image<uint16_t> img = mhsreader.getChannel(i);
                img.equalize();
                img.normalize();
                WRITE_IMAGE(img, directory + "/MHS-" + std::to_string(i + 1) + "-EQU.png");
            }

            // Output a few nice composites as well
            logger->info("Global Composite...");
            image::Image<uint16_t> imageAll(90 * 3, mhsreader.getChannel(0).height() * 2, 1);
            {
                int height = mhsreader.getChannel(0).height();

                // Row 1
                imageAll.draw_image(0, mhsreader.getChannel(0), 90 * 0, 0);
                imageAll.draw_image(0, mhsreader.getChannel(1), 90 * 1, 0);
                imageAll.draw_image(0, mhsreader.getChannel(2), 90 * 2, 0);

                // Row 2
                imageAll.draw_image(0, mhsreader.getChannel(3), 90 * 0, height);
                imageAll.draw_image(0, mhsreader.getChannel(4), 90 * 1, height);
            }
            WRITE_IMAGE(imageAll, directory + "/MHS-ALL.png");

            imageAll = image::Image<uint16_t>(90 * 3, mhsreader.getChannel(0).height() * 2, 1);
            {
                int height = mhsreader.getChannel(0).height();

                image::Image<uint16_t> img1 = mhsreader.getChannel(0);
                image::Image<uint16_t> img2 = mhsreader.getChannel(1);
                image::Image<uint16_t> img3 = mhsreader.getChannel(2);
                image::Image<uint16_t> img4 = mhsreader.getChannel(3);
                image::Image<uint16_t> img5 = mhsreader.getChannel(4);

                img1.equalize();
                img2.equalize();
                img3.equalize();
                img4.equalize();
                img5.equalize();

                img1.normalize();
                img2.normalize();
                img3.normalize();
                img4.normalize();
                img5.normalize();

                // Row 1
                imageAll.draw_image(0, img1, 90 * 0, 0);
                imageAll.draw_image(0, img2, 90 * 1, 0);
                imageAll.draw_image(0, img3, 90 * 2, 0);

                // Row 2
                imageAll.draw_image(0, img4, 90 * 0, height);
                imageAll.draw_image(0, img5, 90 * 1, height);
            }
            WRITE_IMAGE(imageAll, directory + "/MHS-ALL-EQU.png");

            // Reproject to an equirectangular proj
            if (mhsreader.lines > 0)
            {
                // Get satellite info
                nlohmann::json satData = loadJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json");
                int norad = satData.contains("norad") > 0 ? satData["norad"].get<int>() : 0;

                // Setup Projecition
                std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_SCANLINE> proj_settings = geodetic::projection::makeScalineSettingsFromJSON("metop_abc_mhs.json");
                proj_settings->sat_tle = tle::getTLEfromNORAD(norad); // TLEs
                proj_settings->utc_timestamps = mhsreader.timestamps; // Timestamps
                geodetic::projection::LEOScanProjector projector(proj_settings);

                {
                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                    geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/MHS.georef");
                }

                for (int i = 0; i < 5; i++)
                {
                    image::Image<uint16_t> image = mhsreader.getChannel(i);
                    image.equalize();
                    image.normalize();
                    logger->info("Projected channel " + std::to_string(i + 1) + "...");
                    image::Image<uint8_t> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(image, projector, 2048, 1024);
                    WRITE_IMAGE(projected_image, directory + "/MHS-" + std::to_string(i + 1) + "-PROJ.png");
                }
            }
        }

        void MetOpMHSDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("MetOp MHS Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string MetOpMHSDecoderModule::getID()
        {
            return "metop_mhs";
        }

        std::vector<std::string> MetOpMHSDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> MetOpMHSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<MetOpMHSDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace mhs
} // namespace metop