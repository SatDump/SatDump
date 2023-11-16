#include "module_saral_argos.h"
#include <fstream>
#include "common/ccsds/ccsds_weather/demuxer.h"
#include "common/ccsds/ccsds_weather/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace saral
{
    namespace argos
    {
        SaralArgosDecoderModule::SaralArgosDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void SaralArgosDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/ARGOS";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            ccsds::ccsds_weather::Demuxer ccsdsDemuxer(882, true);
            uint64_t argos_cadu = 0, ccsds = 0, argos_ccsds = 0;
            uint8_t cadu[1024];

            logger->info("Demultiplexing and deframing...");
            logger->warn("This will not decode ARGOS frames for now, only dump raw CCSDS frames!");

            std::ofstream frames_out(directory + "/argos.ccsds", std::ios::binary);
            d_output_files.push_back(directory + "/argos.ccsds");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_weather::VCDU vcdu = ccsds::ccsds_weather::parseVCDU(cadu);

                // Right channel? (VCID 1 is ARGOS)
                if (vcdu.vcid == 1)
                {
                    argos_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 36)
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 36)
                        {
                            argos_ccsds++;
                            if (pkt.payload.size() >= 876)
                            {
                                frames_out.write((char *)pkt.header.raw, 6);
                                frames_out.write((char *)pkt.payload.data(), 876);
                            }
                        }
                    }
                }

                progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
                }
            }

            data_in.close();
            frames_out.close();

            logger->info("VCID 1 (ARGOS) Frames  : " + std::to_string(argos_cadu));
            logger->info("CCSDS Frames           : " + std::to_string(ccsds));
            logger->info("ARGOS CCSDS Frames     : " + std::to_string(argos_ccsds));

            logger->info("Writing images.... (Can take a while)");
        }

        void SaralArgosDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Saral ARGOS Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string SaralArgosDecoderModule::getID()
        {
            return "saral_argos";
        }

        std::vector<std::string> SaralArgosDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> SaralArgosDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<SaralArgosDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace avhrr
} // namespace metop