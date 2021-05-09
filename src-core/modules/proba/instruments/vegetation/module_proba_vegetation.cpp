#include "module_proba_vegetation.h"
#include <fstream>
#include "modules/common/ccsds/ccsds_1_0_proba/demuxer.h"
#include "modules/common/ccsds/ccsds_1_0_proba/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"

#include <iostream>

#define WRITE_IMAGE_LOCAL(image, path)         \
    image.save_png(std::string(path).c_str()); \
    all_images.push_back(path);

// Return filesize
size_t getFilesize(std::string filepath);

namespace proba
{
    namespace vegetation
    {
        ProbaVegetationDecoderModule::ProbaVegetationDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void ProbaVegetationDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/Vegetation";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            uint8_t buffer[1279];

            // CCSDS Demuxer
            ccsds::ccsds_1_0_proba::Demuxer ccsdsDemuxer(1103, false);

            logger->info("Demultiplexing and deframing...");

            std::ofstream frames_out(directory + "/veg.ccsds", std::ios::binary);
            d_output_files.push_back(directory + "/veg.ccsds");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 1279);

                int vcid = ccsds::ccsds_1_0_proba::parseVCDU(buffer).vcid;

                std::cout << "VCID" << vcid << std::endl;

                if (vcid == 4)
                {
                    std::vector<ccsds::ccsds_1_0_proba::CCSDSPacket> pkts = ccsdsDemuxer.work(buffer);

                    if (pkts.size() > 0)
                    {
                        for (ccsds::ccsds_1_0_proba::CCSDSPacket pkt : pkts)
                        {
                            if (pkt.header.apid == 2047)
                                continue;

                            std::cout << "APID " << pkt.header.apid << std::endl;

                            if (pkt.header.apid == 273)
                            {
                                std::cout << "LEN " << pkt.payload.size() << std::endl;
                                //frames_out.write((char *)pkt.header.raw, 6);
                                frames_out.write((char *)pkt.payload.data(), 25032);
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

        void ProbaVegetationDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Proba-V Vegetation Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string ProbaVegetationDecoderModule::getID()
        {
            return "proba_vegetation";
        }

        std::vector<std::string> ProbaVegetationDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> ProbaVegetationDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<ProbaVegetationDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace swap
} // namespace proba