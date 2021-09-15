#include "module_jason3_lpt.h"
#include <fstream>
#include "common/ccsds/ccsds_1_0_jason/demuxer.h"
#include "common/ccsds/ccsds_1_0_jason/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "lpt_reader.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace jason3
{
    namespace lpt
    {
        Jason3LPTDecoderModule::Jason3LPTDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void Jason3LPTDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/LPT";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            uint8_t buffer[1279];

            // CCSDS Demuxer
            ccsds::ccsds_1_0_jason::Demuxer ccsdsDemuxer(1101, false);

            logger->info("Demultiplexing and deframing...");

            LPTReader els_a_reader(16 - 6, 22, 64);
            LPTReader els_b_reader(18 - 6, 13, 50);
            LPTReader aps_a_reader(18 - 6, 49, 120);
            LPTReader aps_b_reader(18 - 6, 38, 98);

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 1279);

                int vcid = ccsds::ccsds_1_0_jason::parseVCDU(buffer).vcid;

                if (vcid == 2)
                {
                    std::vector<ccsds::CCSDSPacket> pkts = ccsdsDemuxer.work(buffer);

                    if (pkts.size() > 0)
                    {
                        for (ccsds::CCSDSPacket pkt : pkts)
                        {
                            if (pkt.header.apid == 2047)
                                continue;

                            if (pkt.header.apid == 1316)
                                els_a_reader.work(pkt);
                            if (pkt.header.apid == 1317)
                                els_b_reader.work(pkt);
                            if (pkt.header.apid == 1318)
                                aps_a_reader.work(pkt);
                            if (pkt.header.apid == 1319)
                                aps_b_reader.work(pkt);
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

            logger->info("Writing images.... (Can take a while)");

            // ELS-A
            for (int i = 0; i < 22; i++)
            {
                long min_e = 22e3, max_e = 1.2e6;
                long channel_range = (max_e - min_e) / 22;
                long channel_energy = (min_e + channel_range * i);
                std::string channelName = std::to_string(long(channel_energy / 1e3)) + "keV";

                logger->info("Channel ELS-A " + channelName + "...");
                WRITE_IMAGE(els_a_reader.getImage(i), directory + "/ELS-A-" + channelName + "-MAP.png");
            }

            // ELS-B
            for (int i = 0; i < 13; i++)
            {
                long min_e = 0.4e6, max_e = 19e6;
                long channel_range = (max_e - min_e) / 13;
                long channel_energy = (min_e + channel_range * i);
                std::string channelName = std::to_string(long(channel_energy / 1e3)) + "keV";

                logger->info("Channel ELS-B " + channelName + "...");
                WRITE_IMAGE(els_b_reader.getImage(i), directory + "/ELS-B-" + channelName + "-MAP.png");
            }

            // APS-A
            for (int i = 0; i < 49; i++)
            {
                long min_e = 9.4e6, max_e = 40e6;
                long channel_range = (max_e - min_e) / 49;
                long channel_energy = (min_e + channel_range * i);
                std::string channelName = std::to_string(long(channel_energy / 1e3)) + "keV";

                logger->info("Channel APS-A " + channelName + "...");
                WRITE_IMAGE(aps_a_reader.getImage(i), directory + "/APS-A-" + channelName + "-MAP.png");
            }

            // APS-B
            for (int i = 0; i < 38; i++)
            {
                long min_e = 16e6, max_e = 230e6;
                long channel_range = (max_e - min_e) / 38;
                long channel_energy = (min_e + channel_range * i);
                std::string channelName = std::to_string(long(channel_energy / 1e3)) + "keV";

                logger->info("Channel APS-B " + channelName + "...");
                WRITE_IMAGE(aps_b_reader.getImage(i), directory + "/APS-B-" + channelName + "-MAP.png");
            }
        }

        void Jason3LPTDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Jason-3 LPT Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string Jason3LPTDecoderModule::getID()
        {
            return "jason3_lpt";
        }

        std::vector<std::string> Jason3LPTDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> Jason3LPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<Jason3LPTDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace swap
} // namespace proba