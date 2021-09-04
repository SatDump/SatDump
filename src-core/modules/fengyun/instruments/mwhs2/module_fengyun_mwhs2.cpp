#include "module_fengyun_mwhs2.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "mwhs2_reader.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "nlohmann/json_utils.h"
#include "common/projection/leo_to_equirect.h"
#include "modules/fengyun/fengyun3.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun
{
    namespace mwhs2
    {
        FengyunMWHS2DecoderModule::FengyunMWHS2DecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void FengyunMWHS2DecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWHS-2";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            int vcidFrames = 0, ccsds_frames = 0;

            // Read buffer
            uint8_t buffer[1024];

            logger->info("Demultiplexing and deframing... (WIP!)");

            // Get satellite info
            nlohmann::json satData = loadJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json");
            int norad = satData.contains("norad") > 0 ? satData["norad"].get<int>() : 0;

            // Demuxer
            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer = ccsds::ccsds_1_0_1024::Demuxer(882, true);

            // Reader
            MWHS2Reader mwhs_reader;

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
                            mwhs_reader.work(pkt, norad == FY3_E_NORAD);
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
            logger->info("MWHS-2 Lines           : " + std::to_string(mwhs_reader.lines));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            for (int i = 0; i < 15; i++)
            {
                logger->info("Channel " + std::to_string(i + 1) + "...");
                WRITE_IMAGE(mwhs_reader.getChannel(i), directory + "/MWHS2-" + std::to_string(i + 1) + ".png");
            }

            // Output a few nice composites as well
            logger->info("Global Composite...");
            cimg_library::CImg<unsigned short> imageAll(98 * 4, mwhs_reader.getChannel(0).height() * 4, 1, 1);
            {
                int height = mwhs_reader.getChannel(0).height();

                // Row 1
                imageAll.draw_image(98 * 0, 0, 0, 0, mwhs_reader.getChannel(0));
                imageAll.draw_image(98 * 1, 0, 0, 0, mwhs_reader.getChannel(1));
                imageAll.draw_image(98 * 2, 0, 0, 0, mwhs_reader.getChannel(2));
                imageAll.draw_image(98 * 3, 0, 0, 0, mwhs_reader.getChannel(3));

                // Row 2
                imageAll.draw_image(98 * 0, height, 0, 0, mwhs_reader.getChannel(4));
                imageAll.draw_image(98 * 1, height, 0, 0, mwhs_reader.getChannel(5));
                imageAll.draw_image(98 * 2, height, 0, 0, mwhs_reader.getChannel(6));
                imageAll.draw_image(98 * 3, height, 0, 0, mwhs_reader.getChannel(7));

                // Row 2
                imageAll.draw_image(98 * 0, height * 2, 0, 0, mwhs_reader.getChannel(8));
                imageAll.draw_image(98 * 1, height * 2, 0, 0, mwhs_reader.getChannel(9));
                imageAll.draw_image(98 * 2, height * 2, 0, 0, mwhs_reader.getChannel(10));
                imageAll.draw_image(98 * 3, height * 2, 0, 0, mwhs_reader.getChannel(11));

                // Row 2
                imageAll.draw_image(98 * 0, height * 3, 0, 0, mwhs_reader.getChannel(12));
                imageAll.draw_image(98 * 1, height * 3, 0, 0, mwhs_reader.getChannel(13));
                imageAll.draw_image(98 * 2, height * 3, 0, 0, mwhs_reader.getChannel(14));
            }
            WRITE_IMAGE(imageAll, directory + "/MWHS2-ALL.png");

            // Reproject to an equirectangular proj
            {
                // Setup Projecition
                projection::LEOScanProjector projector(4,                                 // Pixel offset
                                                       1550,                              // Correction swath
                                                       16.0 / 4,                          // Instrument res
                                                       827.0,                             // Orbit height
                                                       2800,                              // Instrument swath
                                                       2.65,                              // Scale
                                                       0,                                 // Az offset
                                                       0,                                 // Tilt
                                                       3,                                 // Time offset
                                                       mwhs_reader.getChannel(0).width(), // Image width
                                                       true,                              // Invert scan
                                                       tle::getTLEfromNORAD(norad),       // TLEs
                                                       mwhs_reader.timestamps             // Timestamps
                );

                for (int i = 0; i < 15; i++)
                {
                    cimg_library::CImg<unsigned short> image = mwhs_reader.getChannel(i);
                    logger->info("Projected Channel " + std::to_string(i + 1) + "...");
                    cimg_library::CImg<unsigned char> projected_image = projection::projectLEOToEquirectangularMapped(image, projector, 2048, 1024);
                    WRITE_IMAGE(projected_image, directory + "/MWHS2-" + std::to_string(i + 1) + "-PROJ.png");
                }
            }
        }

        void FengyunMWHS2DecoderModule::drawUI(bool window)
        {
            ImGui::Begin("FengYun MWHS-2 Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string FengyunMWHS2DecoderModule::getID()
        {
            return "fengyun_mwhs2";
        }

        std::vector<std::string> FengyunMWHS2DecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> FengyunMWHS2DecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<FengyunMWHS2DecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace virr
} // namespace fengyun