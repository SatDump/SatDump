#include "amsu_a1_reader.h"
#include "common/ccsds/ccsds_time.h"

namespace aqua
{
    namespace amsu
    {
        AMSUA1Reader::AMSUA1Reader()
        {
            for (int i = 0; i < 13; i++)
                channels[i].resize(30);
            lines = 0;
        }

        AMSUA1Reader::~AMSUA1Reader()
        {
            for (int i = 0; i < 13; i++)
                channels[i].clear();
        }

        void AMSUA1Reader::work(ccsds::CCSDSPacket &packet)
        {
            // First part of the scan
            if (packet.header.apid == 261)
            {
                if (packet.payload.size() < 704)
                    return;

                int pos = 22;
                for (int i = 0; i < 20 * 17; i++)
                {
                    lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                    pos += 2;
                }

                // Plot into an images
                for (int channel = 0; channel < 13; channel++)
                    for (int i = 0; i < 20; i++)
                        channels[channel][lines * 30 + 29 - i] = lineBuffer[i * 17 + channel];

                timestamps.push_back(ccsds::parseCCSDSTimeFullRawUnsegmented(&packet.payload[1], -4383, 15.3e-6));

                // Frame counter
                lines++;

                for (int i = 0; i < 13; i++)
                    channels[i].resize((lines + 1) * 30);
            }
            // Second part of the scan
            else if (packet.header.apid == 262)
            {
                if (packet.payload.size() < 612)
                    return;

                int pos = 16;
                for (int i = 20 * 17; i < 31 * 17; i++)
                {
                    lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                    pos += 2;
                }

                // Plot into an images
                for (int channel = 0; channel < 13; channel++)
                    for (int i = 20; i < 30; i++)
                        channels[channel][lines * 30 + 29 - i] = lineBuffer[i * 17 + channel];
            }
        }

        image::Image AMSUA1Reader::getChannel(int channel)
        {
            return image::Image(channels[channel].data(), 16, 30, lines, 1);
        }
    } // namespace amsu
} // namespace aqua