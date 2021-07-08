#include "module_metop_sem.h"
#include <fstream>
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/ccsds/ccsds_time.h"
#include "sem_reader.h"
#include "nlohmann/json_utils.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace metop
{
    namespace sem
    {
        MetOpSEMDecoderModule::MetOpSEMDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void MetOpSEMDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/SEM";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer(882, true);
            uint64_t sem_cadu = 0, ccsds = 0, sem_ccsds = 0;
            uint8_t cadu[1024];

            logger->info("Demultiplexing and deframing...");

            // Get satellite info
            nlohmann::json satData = loadJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json");
            int norad = satData.contains("norad") > 0 ? satData["norad"].get<int>() : 0;

            SEMReader reader(norad);

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 12 is MHS)
                if (vcdu.vcid == 3)
                {
                    sem_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 64)
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 37)
                        {
                            reader.work(pkt);
                            sem_ccsds++;
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

            logger->info("VCID 3 (SEM) Frames  : " + std::to_string(sem_cadu));
            logger->info("CCSDS Frames         : " + std::to_string(ccsds));
            logger->info("SEM Frames           : " + std::to_string(sem_ccsds));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Channel 1...");
            WRITE_IMAGE(reader.getImage(), directory + "/SEM-MAP.png");
        }

        void MetOpSEMDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("MetOp SEM Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string MetOpSEMDecoderModule::getID()
        {
            return "metop_sem";
        }

        std::vector<std::string> MetOpSEMDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> MetOpSEMDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<MetOpSEMDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace mhs
} // namespace metop