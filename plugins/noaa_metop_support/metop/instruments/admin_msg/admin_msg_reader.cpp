#include "admin_msg_reader.h"
#include "bzlib/bzlib.h"
#include "logger.h"
#include <fstream>

namespace metop
{
    namespace admin_msg
    {
        AdminMsgReader::AdminMsgReader()
        {
            count = 0;
            message_out = new uint8_t[MAX_MSG_SIZE];
        }

        AdminMsgReader::~AdminMsgReader()
        {
            delete[] message_out;
        }

        void AdminMsgReader::work(ccsds::CCSDSPacket &packet)
        {
            unsigned int outsize = MAX_MSG_SIZE;

            int ret = BZ2_bzBuffToBuffDecompress((char *)message_out, &outsize, (char *)&packet.payload[41], packet.payload.size() - 41, 1, 1);
            if (ret != BZ_OK && ret != BZ_STREAM_END)
            {
                logger->error("Failed decomressing Bzip2 data! Error : " + std::to_string(ret));
                return;
            }

            std::string outputFileName = directory + "/" + std::to_string(packet.header.packet_sequence_count) + ".xml";
            logger->info("Writing message to " + outputFileName);
            std::ofstream outputMessageFile(outputFileName);
            outputMessageFile.write((char *)message_out, outsize);

            count++;
        }
    } // namespace modis
} // namespace eos