#include "module_jpss_cris.h"
#include <fstream>
//#include "atms_reader.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace jpss
{
    namespace cris
    {
        JPSSCrISDecoderModule::JPSSCrISDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                            npp_mode(std::stoi(parameters["npp_mode"]))
        {
            if (npp_mode)
            {
                cadu_size = 1024;
                mpdu_size = 884;
            }
            else
            {
                cadu_size = 1279;
                mpdu_size = 0;
            }
        }

        void JPSSCrISDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/CrIS";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            logger->info("Demultiplexing and deframing...");

            // Read buffer
            uint8_t cadu[1024];

            // Counters
            uint64_t atms_cadu = 0, ccsds = 0, atms_ccsds = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer = ccsds::ccsds_1_0_1024::Demuxer(mpdu_size, false);

            //ATMSReader reader;

            std::ofstream frames_out(directory + "/cris.ccsds", std::ios::binary);
            d_output_files.push_back(directory + "/cris.ccsds");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, cadu_size);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 6 is CrIS)
                if (vcdu.vcid == 6)
                {
                    atms_cadu++;

                    // Demux
                    std::vector<ccsds::ccsds_1_0_1024::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 528)
                    for (ccsds::ccsds_1_0_1024::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 1290)
                        {
                            atms_ccsds++;
                            //reader.work(pkt);
                            logger->info(pkt.payload.size());
                            frames_out.write((char *)pkt.header.raw, 6);
                            frames_out.write((char *)pkt.payload.data(), 7724);
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

            logger->info("VCID 6 (crIS) Frames   : " + std::to_string(atms_cadu));
            logger->info("CCSDS Frames           : " + std::to_string(ccsds));
            logger->info("crIS CCSDS Frames      : " + std::to_string(atms_ccsds));
            //logger->info("ATMS Lines             : " + std::to_string(reader.lines));

            logger->info("Writing images.... (Can take a while)");
        }

        void JPSSCrISDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("JPSS CrIS Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string JPSSCrISDecoderModule::getID()
        {
            return "jpss_cris";
        }

        std::vector<std::string> JPSSCrISDecoderModule::getParameters()
        {
            return {"npp_mode"};
        }

        std::shared_ptr<ProcessingModule> JPSSCrISDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<JPSSCrISDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace atms
} // namespace jpss