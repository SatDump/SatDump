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
        AuraOMIDecoderModule::AuraOMIDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void AuraOMIDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/OMI";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->warn("This decoder is very WIP!!!!");

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            std::ofstream frames_out(directory + "/omi.ccsds", std::ios::binary);
            d_output_files.push_back(directory + "/omi.ccsds");

            time_t lastTime = 0;

            // Read buffer
            uint8_t cadu[1024];

            // Counters
            uint64_t ceres_cadu = 0, ccsds = 0, modis_ccsds = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer1, ccsdsDemuxer2;

            OMIReader reader1, reader2;

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);
                //std::cout << "VCID " << (int)vcdu.vcid << std::endl;
                // Right channel? (VCID 10-15 is CERES)
                if (vcdu.vcid == 26)
                {
                    ceres_cadu++;

                    // Demux
                    std::vector<ccsds::ccsds_1_0_1024::CCSDSPacket> ccsdsFrames = ccsdsDemuxer1.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor
                    for (ccsds::ccsds_1_0_1024::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        //std::cout << "APID " << (int)pkt.header.apid << std::endl;
                        if (pkt.header.apid == 1838 /*+ 2*/)
                        {
                            modis_ccsds++;
                            reader1.work(pkt);
                            //int counter = pkt.payload[9] & 0b11111;
                            //std::cout << "LEN " << (int)pkt.payload.size() << std::endl;
                            //if (counter == 0)
                            //{
                            frames_out.write((char *)pkt.header.raw, 6);
                            frames_out.write((char *)pkt.payload.data(), 4116);
                            //}
                        }
                        else if (pkt.header.apid == 1840)
                        {
                            modis_ccsds++;
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

            logger->info("VCID (CERES) Frames    : " + std::to_string(ceres_cadu));
            logger->info("CCSDS Frames           : " + std::to_string(ccsds));
            logger->info("CERES CCSDS Frames     : " + std::to_string(modis_ccsds));
            //logger->info("CERES 1 Lines          : " + std::to_string(reader1.lines));
            //logger->info("CERES 2 Lines          : " + std::to_string(reader2.lines));

            logger->info("Writing images.... (Can take a while)");

            WRITE_IMAGE(reader1.getImage(0), directory + "/OMI-1.png");
            WRITE_IMAGE(reader2.getImage(0), directory + "/OMI-2.png");
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

        std::shared_ptr<ProcessingModule> AuraOMIDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<AuraOMIDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace ceres
} // namespace aqua