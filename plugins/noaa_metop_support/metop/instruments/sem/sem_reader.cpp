#include "sem_reader.h"
#include "common/ccsds/ccsds_time.h"

namespace metop
{
    namespace sem
    {
        SEMReader::SEMReader()
        {
            samples = 0;
        }

        SEMReader::~SEMReader()
        {
        }

        void SEMReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 656)
                return;

            timestamps.push_back(ccsds::parseCCSDSTime(packet, 10957));

            // Get all samples
            for (int i = 0; i < 640; i += 40)
                for (int j = 0; j < 40; j++)
                    channels[j][i / 40].push_back(packet.payload[i + j] ^ 0xFF);

            samples++;
        }
    } // namespace modis
} // namespace eos