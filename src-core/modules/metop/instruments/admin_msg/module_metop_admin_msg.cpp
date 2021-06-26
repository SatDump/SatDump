#include "module_metop_admin_msg.h"
#include <fstream>
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/bzlib/bzlib.h"

#define MAX_MSG_SIZE 1000000

// Return filesize
size_t getFilesize(std::string filepath);

namespace metop
{
    namespace admin_msg
    {
        MetOpAdminMsgDecoderModule::MetOpAdminMsgDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void MetOpAdminMsgDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/Admin Messages";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer(882, true);
            uint64_t admmsg_cadu = 0, ccsds = 0, admmsg_ccsds = 0;
            uint8_t cadu[1024];

            logger->info("Demultiplexing and deframing...");

            uint8_t *message_out = new uint8_t[MAX_MSG_SIZE];

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 30/42 is MODIS)
                if (vcdu.vcid == 34)
                {
                    admmsg_cadu++;

                    // Demux
                    std::vector<ccsds::ccsds_1_0_1024::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 103 and 104)
                    for (ccsds::ccsds_1_0_1024::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 6)
                        {
                            admmsg_ccsds++;

                            bz_stream bz2_stream;
                            BZ2_bzDecompressInit(&bz2_stream, 1, 1);
                            bz2_stream.next_in = (char *)&pkt.payload[41];
                            bz2_stream.avail_in = pkt.payload.size() - 41;
                            bz2_stream.next_out = (char *)message_out;
                            bz2_stream.avail_out = MAX_MSG_SIZE;

                            int ret = BZ2_bzDecompress(&bz2_stream);
                            if (ret != BZ_OK && ret != BZ_STREAM_END)
                            {
                                logger->error("Failed decomressing Bzip2 data! Error : " + std::to_string(ret));
                                continue;
                            }

                            BZ2_bzDecompressEnd(&bz2_stream);

                            std::string outputFileName = directory + "/" + std::to_string(pkt.header.packet_sequence_count) + ".xml";
                            logger->info("Writing message to " + outputFileName);
                            std::ofstream outputMessageFile(outputFileName);
                            d_output_files.push_back(outputFileName);
                            outputMessageFile.write((char *)message_out, MAX_MSG_SIZE - bz2_stream.avail_out);
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

            logger->info("VCID 34 (Admin) Frames : " + std::to_string(admmsg_cadu));
            logger->info("CCSDS Frames           : " + std::to_string(ccsds));
            logger->info("Messages CCSDS Frames  : " + std::to_string(admmsg_ccsds));

            delete[] message_out;
        }

        void MetOpAdminMsgDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("MetOp Admin Messages Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string MetOpAdminMsgDecoderModule::getID()
        {
            return "metop_admin_msg";
        }

        std::vector<std::string> MetOpAdminMsgDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> MetOpAdminMsgDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<MetOpAdminMsgDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace avhrr
} // namespace metop