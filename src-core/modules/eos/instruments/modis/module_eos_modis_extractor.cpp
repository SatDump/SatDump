#include "module_eos_modis_extractor.h"
#include <fstream>
#include "modis_reader.h"
#include "modules/common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "modules/common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace eos
{
    namespace modis
    {
        EOSMODISExtractorModule::EOSMODISExtractorModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                                terra(std::stoi(parameters["terra_mode"]))
        {
        }

        void EOSMODISExtractorModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MODIS";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            std::ofstream data_out(directory + "/modis_ccsds.bin", std::ios::binary);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);
            logger->info("Writing CCSDS frames to " + directory + "/modis_ccsds.bin");

            time_t lastTime = 0;

            // Read buffer
            uint8_t cadu[1024];

            // CCSDS Buffer
            uint8_t ccsds_modis[642];

            // Counters
            uint64_t modis_cadu = 0, ccsds = 0, modis_ccsds = 0;

            logger->info("Demultiplexing and deframing...");

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 30/42 is MODIS)
                if (vcdu.vcid == (terra ? 42 : 30))
                {
                    modis_cadu++;

                    // Demux
                    std::vector<ccsds::ccsds_1_0_1024::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 64)
                    for (ccsds::ccsds_1_0_1024::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 64)
                        {
                            modis_ccsds++;
                            std::fill(&ccsds_modis[0], &ccsds_modis[642], 0);
                            std::memcpy(&ccsds_modis[0], pkt.header.raw, 6);
                            std::memcpy(&ccsds_modis[6], pkt.payload.data(), std::min<size_t>(pkt.payload.size(), 636));
                            data_out.write((char *)ccsds_modis, 642);
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
            data_out.close();

            logger->info("VCID 30 (MODIS) Frames : " + std::to_string(modis_cadu));
            logger->info("CCSDS Frames           : " + std::to_string(ccsds));
            logger->info("MODIS CCSDS Frames     : " + std::to_string(modis_ccsds));
        }

        void EOSMODISExtractorModule::drawUI(bool window)
        {
            ImGui::Begin("EOS MODIS Extractor", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string EOSMODISExtractorModule::getID()
        {
            return "eos_modis_extractor";
        }

        std::vector<std::string> EOSMODISExtractorModule::getParameters()
        {
            return {"terra_mode"};
        }

        std::shared_ptr<ProcessingModule> EOSMODISExtractorModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<EOSMODISExtractorModule>(input_file, output_file_hint, parameters);
        }
    } // namespace modis
} // namespace eos