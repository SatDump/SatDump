#include "amsu_a2_reader.h"
#include "common/ccsds/ccsds_time.h"

namespace metop
{
    namespace amsu
    {
        AMSUA2Reader::AMSUA2Reader()
        {
            for (int i = 0; i < 2; i++)
                channels[i] = new unsigned short[10000 * 30];
            lines = 0;
        }

        AMSUA2Reader::~AMSUA2Reader()
        {
            for (int i = 0; i < 2; i++)
                delete[] channels[i];
        }

        void AMSUA2Reader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 1136)
                return;

            // This is is very messy, but the spacing between samples wasn't constant so...
            // We have to put it back together the hard way...
            int pos = 42;

            for (int i = 0; i < 15 * 14; i++)
            {
                lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                pos += 2;
            }

            pos = 470 - 6;

            for (int i = 15 * 14; i < 22 * 14; i++)
            {
                lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                pos += 2;
            }

            pos = 668 - 6;

            for (int i = 22 * 14; i < 23 * 14; i++)
            {
                lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                pos += 2;
            }

            pos = 702 - 6;

            for (int i = 23 * 14; i < 24 * 14; i++)
            {
                lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                pos += 2;
            }

            pos = 734 - 6;

            for (int i = 24 * 14; i < 30 * 14; i++)
            {
                lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                pos += 2;
            }

            // Plot into an images
            for (int channel = 0; channel < 2; channel++)
            {
                for (int i = 0; i < 30; i++)
                {
                    uint16_t pixel = lineBuffer[i * 14 + channel];
                    channels[channel][lines * 30 + 29 - i] = pixel;
                }
            }

            timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, 10957));

            // Frame counter
            lines++;
        }

        image::Image<uint16_t> AMSUA2Reader::getChannel(int channel)
        {
            image::Image<uint16_t> img = image::Image<uint16_t>(channels[channel], 30, lines, 1);
            //img.equalize(1000);
            return img;
        }
    } // namespace amsu
} // namespace metop