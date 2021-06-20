#include "module_fengyun_mwts.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "mwts_reader.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun
{
    namespace mwts
    {
        FengyunMWTSDecoderModule::FengyunMWTSDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void FengyunMWTSDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWTS";

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
            MWTSReader mwts_reader;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 1024);

                // Extract VCID
                int vcid = buffer[5] % ((int)pow(2, 6));

                if (vcid == 12)
                {
                    vcidFrames++;

                    std::vector<ccsds::ccsds_1_0_1024::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(buffer);
                    ccsds_frames += ccsdsFrames.size();

                    for (ccsds::ccsds_1_0_1024::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 3)
                            mwts_reader.work(pkt);
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
            logger->info("MWTS Lines             : " + std::to_string(mwts_reader.lines));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            for (int i = 0; i < 26; i++)
            {
                logger->info("Channel " + std::to_string(i + 1) + "...");
                WRITE_IMAGE(mwts_reader.getChannel(i), directory + "/MWTS-" + std::to_string(i + 1) + ".png");
            }

            // Output a few nice composites as well
            logger->info("Global Composite...");
            cimg_library::CImg<unsigned short> imageAll(15 * 26, mwts_reader.getChannel(0).height() * 1, 1, 1);
            {
                for (int i = 0; i < 26; i++)
                {
                    // Row 1
                    imageAll.draw_image(15 * i, 0, 0, 0, mwts_reader.getChannel(i));
                }
            }
            WRITE_IMAGE(imageAll, directory + "/MWTS-ALL.png");
        }

        void FengyunMWTSDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("FengYun MWTS Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string FengyunMWTSDecoderModule::getID()
        {
            return "fengyun_mwts";
        }

        std::vector<std::string> FengyunMWTSDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> FengyunMWTSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<FengyunMWTSDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace virr
} // namespace fengyun