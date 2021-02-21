#include "atms_reader.h"

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

        void ATMSReader::work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet)
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

        cimg_library::CImg<unsigned short> ATMSReader::getImage(int channel)
        {
            return cimg_library::CImg<unsigned short>(channels[channel], 96, lines);
        }
    } // namespace atms
} // namespace jpss