#include "ascat_reader.h"
#include "common/ccsds/ccsds_time.h"
#include <cmath>

namespace metop
{
    namespace ascat
    {
        ASCATReader::ASCATReader()
        {
            for (int i = 0; i < 6; i++)
            {
                channels[i] = new unsigned short[10000 * 256];
                lines[i] = 0;
            }
        }

        ASCATReader::~ASCATReader()
        {
            for (int i = 0; i < 5; i++)
                delete[] channels[i];
        }

        void ASCATReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 654)
                return;

            int channel = packet.header.apid - 208;

            if (channel < 0 || channel > 6)
                return;

            for (int i = 0; i < 256; i++)
            {
                uint16_t sample = packet.payload[140 + i * 2 + 0] << 8 | packet.payload[140 + i * 2 + 1];

                // Details of this format from pyascatreader.cpp
                bool s = (sample >> 15) & 0x1;
                unsigned int e = (sample >> 7) & 0xff;
                unsigned int f = (sample)&0x7f;

                double value = 0;

                if (e == 255)
                {
                    value = 0.0;
                }
                else if (e == 0)
                {
                    if (f == 0)
                        value = 0.0;
                    else
                        value = (s ? -1.0 : 1.0) * pow(2.0, -126.0) * (double)f / 128.0;
                }
                else
                {
                    value = (s ? -1.0 : 1.0) * pow(2.0, double(e - 127)) * ((double)f / 128.0 + 1.0);
                }

                channels[channel][lines[channel] * 256 + i] = value / 100;
            }

            timestamps[channel].push_back(ccsds::parseCCSDSTimeFull(packet, 10957));

            // Frame counter
            lines[channel]++;
        }

        image::Image<uint16_t> ASCATReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel], 256, lines[channel], 1);
        }
    } // namespace avhrr
} // namespace metop