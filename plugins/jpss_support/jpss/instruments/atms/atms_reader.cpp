#include "atms_reader.h"
#include "common/ccsds/ccsds_time.h"

namespace jpss
{
    namespace atms
    {
        ATMSReader::ATMSReader()
        {
            for (int i = 0; i < 22; i++)
                channels[i].resize(96);
            lines = 0;
            scan_pos = -1;
        }

        ATMSReader::~ATMSReader()
        {
            for (int i = 0; i < 22; i++)
                channels[i].clear();
        }

        void ATMSReader::work(ccsds::CCSDSPacket &packet)
        {
            // Filter out bad packets
            if (packet.payload.size() < 56)
                return;

            // Is there a sync signal?
            int scan_synch = packet.payload[10] >> 7;

            // If there is, trigger a new scanline
            if (scan_synch == 1)
            {
                lines++;
                timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, -4383));
                scan_pos = 0;

                for (int i = 0; i < 22; i++)
                    channels[i].resize((lines + 1) * 96);
            }

            // Safeguard
            if (scan_pos < 96 && scan_pos >= 0)
                for (int i = 0; i < 22; i++) // Decode all channels
                    channels[i][(lines * 96) + 95 - scan_pos] = (packet.payload[12 + i * 2] << 8 | packet.payload[13 + i * 2]);

            scan_pos++;
        }

        image::Image<uint16_t> ATMSReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel].data(), 96, lines, 1);
        }
    } // namespace atms
} // namespace jpss