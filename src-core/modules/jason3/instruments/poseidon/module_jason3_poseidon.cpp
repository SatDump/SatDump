#include "module_jason3_poseidon.h"
#include <fstream>
#include "modules/common/ccsds/ccsds_1_0_jason/demuxer.h"
#include "modules/common/ccsds/ccsds_1_0_jason/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"

#include <iostream>

#define WRITE_IMAGE_LOCAL(image, path)         \
    image.save_png(std::string(path).c_str()); \
    all_images.push_back(path);

// Return filesize
size_t getFilesize(std::string filepath);

namespace jason3
{
    namespace poseidon
    {
        Jason3PoseidonDecoderModule::Jason3PoseidonDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void Jason3PoseidonDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/Poseidon";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            uint8_t buffer[1279];

            // CCSDS Demuxer
            ccsds::ccsds_1_0_jason::Demuxer ccsdsDemuxer(1101, false);

            logger->info("Demultiplexing and deframing...");

            std::ofstream frames_out(directory + "/pos.ccsds", std::ios::binary);
            d_output_files.push_back(directory + "/pos.ccsds");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 1279);

                int vcid = ccsds::ccsds_1_0_jason::parseVCDU(buffer).vcid;

                std::cout << "VCID" << vcid << std::endl;

                if (vcid == 1)
                {
                    std::vector<ccsds::ccsds_1_0_jason::CCSDSPacket> pkts = ccsdsDemuxer.work(buffer);

                    if (pkts.size() > 0)
                    {
                        for (ccsds::ccsds_1_0_jason::CCSDSPacket pkt : pkts)
                        {
                            if (pkt.header.apid == 2047)
                                continue;

                            std::cout << "APID " << pkt.header.apid << std::endl;

                            if (pkt.header.apid == 1031)
                            {
                                std::cout << "LEN " << pkt.payload.size() << std::endl;
                                frames_out.write((char *)pkt.header.raw, 6);
                                frames_out.write((char *)pkt.payload.data(), 930);
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

            frames_out.close();
            //d_output_files = swap_reader.all_images;
        }

        void Jason3PoseidonDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Jason-3 Poseidon-3 Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20));

            ImGui::End();
        }

        std::string Jason3PoseidonDecoderModule::getID()
        {
            return "jason3_poseidon";
        }

        std::vector<std::string> Jason3PoseidonDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> Jason3PoseidonDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<Jason3PoseidonDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace swap
} // namespace proba