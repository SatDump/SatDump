#include "module_saral_argos.h"
#include "common/ccsds/ccsds_aos/demuxer.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

#define BUFFER_SIZE 8192

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace saral
{
    namespace argos
    {
        SaralArgosDecoderModule::SaralArgosDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
        }

        void SaralArgosDecoderModule::process()
        {
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/ARGOS";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            ccsds::ccsds_aos::Demuxer ccsdsDemuxer(882, true);
            uint64_t argos_cadu = 0, ccsds = 0, argos_ccsds = 0;
            uint8_t cadu[1024];

            logger->info("Demultiplexing and deframing...");
            logger->warn("This will not decode ARGOS frames for now, only dump raw CCSDS frames!");

            std::ofstream frames_out(directory + "/argos.ccsds", std::ios::binary);

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);

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
            }

            cleanup();
            frames_out.close();

            logger->info("VCID 1 (ARGOS) Frames  : " + std::to_string(argos_cadu));
            logger->info("CCSDS Frames           : " + std::to_string(ccsds));
            logger->info("ARGOS CCSDS Frames     : " + std::to_string(argos_ccsds));

            logger->info("Writing images.... (Can take a while)");
        }

        void SaralArgosDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Saral ARGOS Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            drawProgressBar();

            ImGui::End();
        }

        std::string SaralArgosDecoderModule::getID() { return "saral_argos"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> SaralArgosDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<SaralArgosDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace argos
} // namespace saral