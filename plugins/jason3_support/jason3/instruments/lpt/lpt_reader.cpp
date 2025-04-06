#define cimg_use_jpeg
#include "lpt_reader.h"
#include "resources.h"
// #include "tle.h"
#include "common/ccsds/ccsds_time.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

namespace jason3
{
    namespace lpt
    {
        LPTReader::LPTReader(int start_byte, int channel_count, int pkt_size) : start_byte(start_byte),
                                                                                channel_count(channel_count),
                                                                                pkt_size(pkt_size)
        {
            frames = 0;
            channel_counts.resize(channel_count);
        }

        LPTReader::~LPTReader()
        {
        }

        void LPTReader::work(ccsds::CCSDSPacket &packet)
        {
            if ((int)packet.payload.size() < pkt_size)
                return;

            frames++;

            // We need to know where the satellite was when that packet was created
            double currentTime = ccsds::parseCCSDSTimeFull(packet, 16743 + 1056, 1);
            timestamps.push_back(currentTime);

            for (int ch = 0; ch < channel_count; ch++)
            {
                // Get all samples
                uint16_t sample = packet.payload[start_byte + 2 * ch + 0] << 8 | packet.payload[start_byte + 2 * ch + 1];
                channel_counts[ch].push_back(sample);
            }
        }
    } // namespace modis
} // namespace eos