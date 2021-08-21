#include "mwts3_reader.h"
#include "logger.h"

namespace fengyun
{
    namespace mwts3
    {
        MWTS3Reader::MWTS3Reader()
        {
            for (int i = 0; i < 18; i++)
                channels[i].create(10000 * 98);

            lines = 0;
        }

        MWTS3Reader::~MWTS3Reader()
        {
            for (int i = 0; i < 18; i++)
                channels[i].destroy();
        }

        void MWTS3Reader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 1018)
                return;

            int marker = (packet.payload[0] >> 4) & 0b111;

            if (marker == 1)
            {
                int pos = 224 + 144 * 2;
                for (int i = 0; i < 14; i++)
                {
                    for (int c = 0; c < 18; c++)
                        channels[c][lines * 98 + i] = packet.payload[pos + (18 * i + c) * 2 + 0] << 8 | packet.payload[pos + (18 * i + c) * 2 + 1];
                }

                lines++;
            }
            else if (marker == 2)
            {
                int pos = 8;
                for (int i = 0; i < 28; i++)
                {
                    for (int c = 0; c < 18; c++)
                        channels[c][lines * 98 + 14 + i] = packet.payload[pos + (18 * i + c) * 2 + 0] << 8 | packet.payload[pos + (18 * i + c) * 2 + 1];
                }
            }
            else if (marker == 3)
            {
                int pos = 8;
                for (int i = 0; i < 28; i++)
                {
                    for (int c = 0; c < 18; c++)
                        channels[c][lines * 98 + 42 + i] = packet.payload[pos + (18 * i + c) * 2 + 0] << 8 | packet.payload[pos + (18 * i + c) * 2 + 1];
                }
            }
            else if (marker == 4)
            {
                int pos = 8;
                for (int i = 0; i < 28; i++)
                {
                    for (int c = 0; c < 18; c++)
                        channels[c][lines * 98 + 70 + i] = packet.payload[pos + (18 * i + c) * 2 + 0] << 8 | packet.payload[pos + (18 * i + c) * 2 + 1];
                }
            }

            // Make sure we have enough room
            if (lines * 98 >= (int)channels[0].size())
            {
                for (int i = 0; i < 18; i++)
                    channels[i].resize((lines + 1000) * 98);
            }
        }

        cimg_library::CImg<unsigned short> MWTS3Reader::getChannel(int channel)
        {
            cimg_library::CImg<unsigned short> image(channels[channel].buf, 98, lines);
            image.normalize(0, 65535);
            image.equalize(1000);
            return image;
        }
    }
}