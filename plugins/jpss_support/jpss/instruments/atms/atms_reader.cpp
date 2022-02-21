#include "atms_reader.h"
#include "common/ccsds/ccsds_time.h"

namespace jpss
{
    namespace atms
    {
        ATMSReader::ATMSReader()
        {
            for (int i = 0; i < 22; i++)
                channels[i] = new unsigned short[10000 * 96];

            lines = 0;
            inScan = false;
        }

        ATMSReader::~ATMSReader()
        {
            for (int i = 0; i < 22; i++)
                delete[] channels[i];
        }

        void ATMSReader::work(ccsds::CCSDSPacket &packet)
        {
            // Filter out bad packets
            if (packet.payload.size() < 55)
                return;

            // Is there a sync signal?
            int scan_synch = packet.payload[10] >> 7;

            // If there is, trigger a new scanline
            if (scan_synch == 1)
            {
                lines++;
                timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, -4383));

                endSequenceCount = packet.header.packet_sequence_count + 96;
                inScan = true;
            }

            // If we're in a scan
            if (inScan && endSequenceCount > packet.header.packet_sequence_count)
            {
                // Compute the scan pos, 0 - 96
                int scan_pos = endSequenceCount - packet.header.packet_sequence_count - 1;

                // Safeguard
                if (scan_pos < 96)
                {
                    // Decode all channels
                    for (int i = 0; i < 22; i++)
                        channels[i][(lines * 96) + scan_pos] = (packet.payload[12 + i * 2] << 8 | packet.payload[13 + i * 2]) * 2;
                }
            }
            else
            {
                inScan = false;
            }
        }

        image::Image<uint16_t> ATMSReader::getImage(int channel)
        {
            return image::Image<uint16_t>(channels[channel], 96, lines, 1);
        }
    } // namespace atms
} // namespace jpss