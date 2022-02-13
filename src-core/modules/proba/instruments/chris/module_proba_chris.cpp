#include "module_proba_chris.h"
#include <fstream>
#include "chris_reader.h"
#include "common/ccsds/ccsds_1_0_proba/demuxer.h"
#include "common/ccsds/ccsds_1_0_proba/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace proba
{
    namespace chris
    {
        ProbaCHRISDecoderModule::ProbaCHRISDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void ProbaCHRISDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/CHRIS";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            uint8_t buffer[1279];

            // CCSDS Demuxer
            ccsds::ccsds_1_0_proba::Demuxer ccsdsDemuxer(1103, false);

            CHRISReader chris_reader(directory);

            chris_reader.composites_all = d_parameters["composites_all"];
            chris_reader.composites_low = d_parameters["composites_low"];

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 1279);

                int vcid = ccsds::ccsds_1_0_proba::parseVCDU(buffer).vcid;

                if (vcid == 1)
                {
                    std::vector<ccsds::CCSDSPacket> pkts = ccsdsDemuxer.work(buffer);

                    if (pkts.size() > 0)
                    {
                        for (ccsds::CCSDSPacket pkt : pkts)
                        {
                            if (pkt.header.apid == 2047)
                                continue;

                            if (pkt.header.apid == 0)
                            {
                                int mode_marker = pkt.payload[9] & 0x03;
                                if (mode_marker != 1)
                                    chris_reader.work(pkt);

                                //if (pkt.payload.size() >= 11538)
                                //    data_out.write((char *)pkt.payload.data(), 11538);
                            }
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

            chris_reader.save();

            d_output_files = chris_reader.all_images;
        }

        void ProbaCHRISDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Proba-1 CHRIS Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string ProbaCHRISDecoderModule::getID()
        {
            return "proba_chris";
        }

        std::vector<std::string> ProbaCHRISDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> ProbaCHRISDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<ProbaCHRISDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace chris
} // namespace proba