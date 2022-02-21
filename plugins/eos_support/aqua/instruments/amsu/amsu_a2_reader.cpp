#include "amsu_a2_reader.h"
#include "common/ccsds/ccsds_time.h"

namespace aqua
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
            if (packet.payload.size() < 350)
                return;

            // This is is very messy, but the spacing between samples wasn't constant so...
            // We have to put it back together the hard way...
            int pos = 18;

            for (int i = 0; i < 30 * 4; i++)
            {
                lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                pos += 2;
            }

            // Plot into an images
            for (int channel = 0; channel < 2; channel++)
            {
                for (int i = 0; i < 30; i++)
                {
                    channels[channel][lines * 30 + 30 - i] = lineBuffer[i * 4 + channel];
                }
            }

            double timestamp = ccsds::parseCCSDSTimeFullRawUnsegmented(&packet.payload[1], -4383, 15.3e-6);
            timestamps.push_back(timestamp);

            // Frame counter
            lines++;
        }

        image::Image<uint16_t> AMSUA2Reader::getChannel(int channel)
        {
            image::Image<uint16_t> img = image::Image<uint16_t>(channels[channel], 30, lines, 1);
            img.normalize();
            img.equalize();
            return img;
        }
    } // namespace amsu
} // namespace aqua