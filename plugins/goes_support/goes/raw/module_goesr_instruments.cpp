#include "module_goesr_instruments.h"
#include <fstream>
#include "common/ccsds/ccsds_weather/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/ccsds/ccsds_weather/demuxer.h"
#include "products/products.h"
#include "products/image_products.h"
#include "products/dataset.h"
#include "resources.h"

namespace goes
{
    namespace instruments
    {
        GOESRInstrumentsDecoderModule::GOESRInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void GOESRInstrumentsDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;
            uint8_t cadu[1024];

            // Demuxers
            ccsds::ccsds_weather::Demuxer demuxer_vcid1(884, true, 0);
            ccsds::ccsds_weather::Demuxer demuxer_vcid2(884, true, 0);

            // Setup readers
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

            {
                std::string suvi_dir = directory + "/SUVI";
                if(!std::filesystem::exists(suvi_dir))
                    std::filesystem::create_directories(suvi_dir);
                suvi_reader.directory = suvi_dir;
            }

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_weather::VCDU vcdu = ccsds::ccsds_weather::parseVCDU(cadu);

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

                progress = data_in.tellg();
                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
                }
            }

            data_in.close();
        }

        void GOESRInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("GOES-R Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string GOESRInstrumentsDecoderModule::getID()
        {
            return "goesr_instruments";
        }

        std::vector<std::string> GOESRInstrumentsDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> GOESRInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GOESRInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace metop