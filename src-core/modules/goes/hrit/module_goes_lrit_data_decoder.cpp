#include "module_goes_lrit_data_decoder.h"
#include <fstream>
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "data/lrit_data_decoder.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace goes
{
    namespace hrit
    {
        GOESLRITDataDecoderModule::GOESLRITDataDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        std::vector<ModuleDataType> GOESLRITDataDecoderModule::getInputTypes()
        {
            return {DATA_FILE, DATA_STREAM};
        }

        std::vector<ModuleDataType> GOESLRITDataDecoderModule::getOutputTypes()
        {
            return {DATA_FILE};
        }

        void GOESLRITDataDecoderModule::process()
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

            uint8_t cadu[1024];

            logger->info("Demultiplexing and deframing...");

            std::map<int, std::shared_ptr<ccsds::ccsds_1_0_1024::Demuxer>> demuxers;
            std::map<int, std::shared_ptr<LRITDataDecoder>> decoders;

            while (!data_in.eof())
            {
                // Read buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)&cadu, 1024);
                else
                    input_fifo->read((uint8_t *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                if (demuxers.count(vcdu.vcid) <= 0)
                    demuxers.emplace(std::pair<int, std::shared_ptr<ccsds::ccsds_1_0_1024::Demuxer>>(vcdu.vcid, std::make_shared<ccsds::ccsds_1_0_1024::Demuxer>(884, false)));

                // Demux
                std::vector<ccsds::ccsds_1_0_1024::CCSDSPacket> ccsdsFrames = demuxers[vcdu.vcid]->work(cadu);

                // Push into processor (filtering APID 103 and 104)
                for (ccsds::ccsds_1_0_1024::CCSDSPacket &pkt : ccsdsFrames)
                {
                    if (pkt.header.apid != 2047)
                    {
                        if (decoders.count(vcdu.vcid) <= 0)
                            decoders.emplace(std::pair<int, std::shared_ptr<LRITDataDecoder>>(vcdu.vcid, std::make_shared<LRITDataDecoder>(directory)));
                        decoders[vcdu.vcid]->work(pkt);
                    }
                }

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            for (const std::pair<const int, std::shared_ptr<LRITDataDecoder>> &dec : decoders)
                dec.second->save();
        }

        void GOESLRITDataDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("GOES HRIT Data Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string GOESLRITDataDecoderModule::getID()
        {
            return "goes_lrit_data_decoder";
        }

        std::vector<std::string> GOESLRITDataDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> GOESLRITDataDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<GOESLRITDataDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace avhrr
} // namespace metop