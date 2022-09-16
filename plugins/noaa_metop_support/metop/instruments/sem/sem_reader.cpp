#include "sem_reader.h"
#include "common/ccsds/ccsds_time.h"

#include "logger.h"

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

            double timestamp = ccsds::parseCCSDSTime(packet, 10957);
            for (int i = 0; i < 32; i += 2) // 32 seconds intervals between packets. 2-seconds per "exposure"
                timestamps.push_back(timestamp + i);

            // Get all samples
            for (int i = 0; i < 640; i += 40)
                for (int j = 0; j < 40; j++)
                    channels[j].push_back(packet.payload[15 + i + j] ^ 0xFF);

            samples++;
        }
    } // namespace modis
} // namespace eos