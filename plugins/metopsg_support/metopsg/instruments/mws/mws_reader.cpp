#include "mws_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"
#include "common/tracking/interpolator.h"
#include "image/image.h"
#include <cstdio>

namespace metopsg
{
    namespace mws
    {
        MWSReader::MWSReader()
        {
            for (int c = 0; c < 24; c++)
                lines[c] = 0;
        }

        MWSReader::~MWSReader() {}

        void MWSReader::work(ccsds::CCSDSPacket &pkt)
        {
            if (pkt.header.apid == 1105 || pkt.header.apid == 1106)
            {
                int ch = pkt.header.apid - 1105;
                for (int i = 0; i < 1680; i++)
                    channels[ch].push_back(uint16_t(pkt.payload[28 + i * 2 + 0] << 8 | pkt.payload[28 + i * 2 + 1]) - 32768);
                lines[ch]++;
            }
            else if (pkt.header.apid == 1107)
            {
                for (int c = 0; c < 14; c++)
                {
                    for (int i = 0; i < 105; i++)
                        channels[2 + c].push_back(uint16_t(pkt.payload[28 + c * 105 * 2 + i * 2 + 0] << 8 | pkt.payload[28 + c * 105 * 2 + i * 2 + 1]) - 32768);
                    lines[2 + c]++;
                }
            }
            else if (pkt.header.apid == 1108)
            {
                for (int i = 0; i < 105; i++)
                    channels[16].push_back(uint16_t(pkt.payload[28 + i * 2 + 0] << 8 | pkt.payload[28 + i * 2 + 1]) - 32768);
                lines[16]++;
            }
            else if (pkt.header.apid == 1109)
            {
                for (int i = 0; i < 105; i++)
                    channels[17].push_back(uint16_t(pkt.payload[28 + i * 2 + 0] << 8 | pkt.payload[28 + i * 2 + 1]) - 32768);
                lines[17]++;
            }
            else if (pkt.header.apid == 1110)
            {
                for (int c = 0; c < 5; c++)
                {
                    for (int i = 0; i < 105; i++)
                        channels[18 + c].push_back(uint16_t(pkt.payload[28 + c * 105 * 2 + i * 2 + 0] << 8 | pkt.payload[28 + c * 105 * 2 + i * 2 + 1]) - 32768);
                    lines[18 + c]++;
                }
            }
            else if (pkt.header.apid == 1111)
            {
                for (int i = 0; i < 105; i++)
                    channels[23].push_back(uint16_t(pkt.payload[28 + i * 2 + 0] << 8 | pkt.payload[28 + i * 2 + 1]) - 32768);
                lines[23]++;
            }
        }

        image::Image MWSReader::getChannel(int c)
        {
            if (c == 0 || c == 1)
            {
                image::Image img(channels[c].data(), 16, 1680, lines[c], 1);
                img.crop(0, 0, 1520, img.height());
                return img;
            }
            else
            {
                image::Image img(channels[c].data(), 16, 105, lines[c], 1);
                img.crop(0, 0, 95, img.height());
                return img;
            }
        }
    } // namespace mws
} // namespace metopsg