#include "module_aura_omi.h"
#include <fstream>
#include "omi_reader.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include <iostream>

// Return filesize
size_t getFilesize(std::string filepath);

namespace aura
{
    namespace omi
    {
        AuraOMIDecoderModule::AuraOMIDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void AuraOMIDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/OMI";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            // Read buffer
            uint8_t cadu[1024];

            // Counters
            uint64_t omi_cadu = 0, ccsds = 0, omi_ccsds = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer1, ccsdsDemuxer2;

            OMIReader reader1, reader2;

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel?
                if (vcdu.vcid == 26)
                {
                    omi_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer1.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 1838)
                        {
                            omi_ccsds++;
                            reader1.work(pkt);
                        }
                        else if (pkt.header.apid == 1840)
                        {
                            omi_ccsds++;
                            reader2.work(pkt);
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

            logger->info("VCID 26 (OMI) Frames   : " + std::to_string(omi_cadu));
            logger->info("CCSDS Frames           : " + std::to_string(ccsds));
            logger->info("OMI CCSDS Frames       : " + std::to_string(omi_ccsds));
            logger->info("OMI UV 1 Lines         : " + std::to_string(reader1.lines));
            logger->info("OMI VIS Lines          : " + std::to_string(reader2.lines));

            logger->info("Writing images.... (Can take a while)");

            WRITE_IMAGE(reader1.getImageRaw(), directory + "/OMI-1.png");
            WRITE_IMAGE(reader2.getImageRaw(), directory + "/OMI-2.png");

            WRITE_IMAGE(reader1.getImageVisible(), directory + "/OMI-VIS-1.png");
            WRITE_IMAGE(reader2.getImageVisible(), directory + "/OMI-VIS-2.png");

            logger->info("Global Composite...");
            int all_width_count = 33;
            int all_height_count = 24;
            image::Image<uint16_t> imageAll1(65 * all_width_count, reader1.getChannel(0).height() * all_height_count, 1);
            {
                int height = reader1.getChannel(0).height();

                for (int row = 0; row < all_height_count; row++)
                {
                    for (int column = 0; column < all_width_count; column++)
                    {
                        if (row * all_width_count + column >= 792)
                            break;

                        imageAll1.draw_image(0, reader1.getChannel(row * all_width_count + column), 65 * column, height * row);
                    }
                }
            }
            image::Image<uint16_t> imageAll2(65 * all_width_count, reader2.getChannel(0).height() * all_height_count, 1);
            {
                int height = reader2.getChannel(0).height();

                for (int row = 0; row < all_height_count; row++)
                {
                    for (int column = 0; column < all_width_count; column++)
                    {
                        if (row * all_width_count + column >= 792)
                            break;

                        imageAll2.draw_image(0, reader2.getChannel(row * all_width_count + column), 65 * column, height * row);
                    }
                }
            }
            WRITE_IMAGE(imageAll1, directory + "/OMI-ALL-1.png");
            WRITE_IMAGE(imageAll2, directory + "/OMI-ALL-2.png");
        }

        void AuraOMIDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Aura OMI Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string AuraOMIDecoderModule::getID()
        {
            return "aura_omi";
        }

        std::vector<std::string> AuraOMIDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> AuraOMIDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<AuraOMIDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace ceres
} // namespace aqua