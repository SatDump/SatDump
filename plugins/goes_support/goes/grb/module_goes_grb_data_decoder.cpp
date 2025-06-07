#include "module_goes_grb_data_decoder.h"
#include "common/ccsds/ccsds_aos/demuxer.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "data/payload_assembler.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "logger.h"
#include <filesystem>
#include <fstream>
#include <iostream>

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace goes
{
    namespace grb
    {
        GOESGRBDataDecoderModule::GOESGRBDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
        }

        GOESGRBDataDecoderModule::~GOESGRBDataDecoderModule() {}

        void GOESGRBDataDecoderModule::process()
        {
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            uint8_t cadu[2048];

            logger->info("Demultiplexing and deframing...");

            ccsds::ccsds_aos::Demuxer demuxer_rhcp(2034, false);
            ccsds::ccsds_aos::Demuxer demuxer_lhcp(2034, false);

            image::ImageSavingThread image_saving_th(input_data_type != satdump::pipeline::DATA_FILE);

            GRBFilePayloadAssembler assember_rhcp;
            GRBFilePayloadAssembler assember_lhcp;

            assember_rhcp.ignore_crc = assember_lhcp.ignore_crc = d_parameters.contains("ignore_crc") ? d_parameters["ignore_crc"].get<bool>() : false;
            assember_rhcp.processor = assember_lhcp.processor = std::make_shared<GRBDataProcessor>(directory, &image_saving_th);

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)&cadu, 2048);

                // Parse this transport frame
                ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);

                if (vcdu.vcid == 63)
                    continue;

                if (vcdu.vcid == 5) // RHCP
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_rhcp.work(cadu);

                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 2047)
                            continue;

                        assember_rhcp.work(pkt);
                    }
                }
                else if (vcdu.vcid == 6) // LHCP
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_lhcp.work(cadu);

                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 2047)
                            continue;

                        assember_lhcp.work(pkt);
                    }
                }
            }

            cleanup();
        }

        void GOESGRBDataDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("GOES GRB Data Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            drawProgressBar();

            ImGui::End();
        }

        std::string GOESGRBDataDecoderModule::getID() { return "goes_grb_data_decoder"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> GOESGRBDataDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GOESGRBDataDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace grb
} // namespace goes