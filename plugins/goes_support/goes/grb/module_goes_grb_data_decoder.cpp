#include "module_goes_grb_data_decoder.h"
#include <fstream>
#include "common/ccsds/ccsds_weather/demuxer.h"
#include "common/ccsds/ccsds_weather/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include <iostream>
#include "data/payload_assembler.h"

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace goes
{
    namespace grb
    {
        GOESGRBDataDecoderModule::GOESGRBDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        std::vector<ModuleDataType> GOESGRBDataDecoderModule::getInputTypes()
        {
            return {DATA_FILE, DATA_STREAM};
        }

        std::vector<ModuleDataType> GOESGRBDataDecoderModule::getOutputTypes()
        {
            return {DATA_FILE};
        }

        GOESGRBDataDecoderModule::~GOESGRBDataDecoderModule()
        {
        }

        void GOESGRBDataDecoderModule::process()
        {
            std::ifstream data_in;

            if (input_data_type == DATA_FILE)
                filesize = getFilesize(d_input_file);
            else
                filesize = 0;
            if (input_data_type == DATA_FILE)
                data_in = std::ifstream(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            uint8_t cadu[2048];

            logger->info("Demultiplexing and deframing...");

            ccsds::ccsds_weather::Demuxer demuxer_rhcp(2034, false);
            ccsds::ccsds_weather::Demuxer demuxer_lhcp(2034, false);

            image::ImageSavingThread image_saving_th(input_data_type != DATA_FILE);

            GRBFilePayloadAssembler assember_rhcp;
            GRBFilePayloadAssembler assember_lhcp;

            assember_rhcp.ignore_crc = assember_lhcp.ignore_crc = d_parameters.contains("ignore_crc") ? d_parameters["ignore_crc"].get<bool>() : false;
            assember_rhcp.processor = assember_lhcp.processor = std::make_shared<GRBDataProcessor>(directory, &image_saving_th);

            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                // Read buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)&cadu, 2048);
                else
                    input_fifo->read((uint8_t *)&cadu, 2048);

                // Parse this transport frame
                ccsds::ccsds_weather::VCDU vcdu = ccsds::ccsds_weather::parseVCDU(cadu);

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

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
                }
            }

            data_in.close();
        }

        void GOESGRBDataDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("GOES GRB Data Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (!streamingInput)
                ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string GOESGRBDataDecoderModule::getID()
        {
            return "goes_grb_data_decoder";
        }

        std::vector<std::string> GOESGRBDataDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> GOESGRBDataDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GOESGRBDataDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace avhrr
} // namespace metop