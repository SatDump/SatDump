#include "module_fengyun_mwhs.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "mwhs_reader.h"
#include "modules/common/ccsds/ccsds_1_0_1024/demuxer.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun
{
    namespace mwhs
    {
        FengyunMWHSDecoderModule::FengyunMWHSDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
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

            logger->info("Demultiplexing and deframing...");

            // Demuxer
            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer = ccsds::ccsds_1_0_1024::Demuxer(882, true);

            // Reader
            MWHSReader mwhs_reader;
            std::ofstream frames("test.bin");
            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 1024);

                // Extract VCID
                int vcid = buffer[5] % ((int)pow(2, 6));
                //logger->debug(vcid);
                if (vcid == 12)
                {
                    vcidFrames++;

                    std::vector<ccsds::ccsds_1_0_1024::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(buffer);
                    ccsds_frames += ccsdsFrames.size();

                    for (ccsds::ccsds_1_0_1024::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        // logger->debug("APID " + std::to_string(pkt.header.apid));

                        if (pkt.header.apid == 16)
                        {
                            //logger->debug("SIZE " + std::to_string(pkt.payload.size()));
                            if ((pkt.payload[350] & 2) == 0)
                                frames.write((char *)pkt.payload.data(), 1018);
                            mwhs_reader.work(pkt);
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
            //logger->info("ERM Lines              : " + std::to_string(erm_reader.lines));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Channel 1...");
            WRITE_IMAGE(mwhs_reader.getChannel(0), directory + "/MWHS-1.png");

            logger->info("Channel 2...");
            WRITE_IMAGE(mwhs_reader.getChannel(1), directory + "/MWHS-2.png");

            logger->info("Channel 3...");
            WRITE_IMAGE(mwhs_reader.getChannel(2), directory + "/MWHS-3.png");

            logger->info("Channel 4...");
            WRITE_IMAGE(mwhs_reader.getChannel(3), directory + "/MWHS-4.png");

            logger->info("Channel 5...");
            WRITE_IMAGE(mwhs_reader.getChannel(4), directory + "/MWHS-5.png");

            logger->info("Channel 6...");
            WRITE_IMAGE(mwhs_reader.getChannel(5), directory + "/MWHS-6.png");
        }

        void FengyunMWHSDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("FengYun MWHS Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20));

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

        std::shared_ptr<ProcessingModule> FengyunMWHSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<FengyunMWHSDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace virr
} // namespace fengyun