#include "module_goesr_instruments.h"
#include "common/ccsds/ccsds_aos/demuxer.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "common/utils.h"
#include "core/resources.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace goes
{
    namespace instruments
    {
        GOESRInstrumentsDecoderModule::GOESRInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
        }

        void GOESRInstrumentsDecoderModule::process()
        {
            uint8_t cadu[1024];

            // Demuxers
            ccsds::ccsds_aos::Demuxer demuxer_vcid1(884, true, 0);
            ccsds::ccsds_aos::Demuxer demuxer_vcid2(884, true, 0);

            // Setup readers
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

            {
                std::string suvi_dir = directory + "/SUVI";
                if (!std::filesystem::exists(suvi_dir))
                    std::filesystem::create_directories(suvi_dir);
                suvi_reader.directory = suvi_dir;
            }

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);

                if (vcdu.vcid == 1) // ABI
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid1.work(cadu);
                    // for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    //     if (pkt.header.apid == 528)
                    //         atms_reader.work(pkt);
                }
                else if (vcdu.vcid == 2) // All the rest
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid2.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        if (pkt.header.apid == 810)
                            suvi_reader.work(pkt);
                }
            }

            cleanup();
        }

        void GOESRInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("GOES-R Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            drawProgressBar();

            ImGui::End();
        }

        std::string GOESRInstrumentsDecoderModule::getID() { return "goesr_instruments"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> GOESRInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GOESRInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace instruments
} // namespace goes