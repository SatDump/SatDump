#include "mwhs_reader.h"

namespace fengyun
{
    namespace mwhs
    {
        MWHSReader::MWHSReader()
        {
            imageVector.push_back(MWHSImage());
            lines = 6;
        }

        MWHSReader::~MWHSReader()
        {
        }

        void MWHSReader::work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 1018)
                return;

            int marker = packet.payload[350] & 2;

            int counter = packet.payload[79] & 0b00011111;
            int mk = (packet.payload[79] >> 5) & 1;

            int pos = 172 + 30 * 6;

            int shift = 3;

            for (int i = 0; i < 500; i++)
            {
                byteBufShift[0] = packet.payload[pos + i * 2 + 0] << shift | packet.payload[pos + i * 2 + 1] >> (8 - shift);
                byteBufShift[1] = packet.payload[pos + i * 2 + 1] << shift | packet.payload[pos + i * 2 + 2] >> (8 - shift);

                lineBuf[i] = byteBufShift[0] << 8 | byteBufShift[1];
            }

            if (imageVector[imageVector.size() - 1].mk == -1)
                imageVector[imageVector.size() - 1].mk = mk;

            if (mk == imageVector[imageVector.size() - 1].mk)
            {
                imageVector[imageVector.size() - 1].lastMkMatch = counter;
            }

            for (int i = 0; i < 49; i++)
            {
                imageVector[imageVector.size() - 1].channels[0][counter * 99 + (marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 0];
                imageVector[imageVector.size() - 1].channels[1][counter * 99 + (marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 1];
                imageVector[imageVector.size() - 1].channels[2][counter * 99 + (marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 2];
                imageVector[imageVector.size() - 1].channels[3][counter * 99 + (marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 3];
                imageVector[imageVector.size() - 1].channels[4][counter * 99 + (marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 4];
                imageVector[imageVector.size() - 1].channels[5][counter * 99 + (marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 5];
            }

            for (int i = 0; i < 6; i++)
            {
                if (mk == 1)
                    imageVector[imageVector.size() - 1].channels[i][counter * 99 + 98] = 65535;
                else
                    imageVector[imageVector.size() - 1].channels[i][counter * 99 + 98] = 0;
            }

            if (counter == 31 && marker == 0)
            {
                imageVector.push_back(MWHSImage());
                lines += 6;
            }

            lines++;
        }

        cimg_library::CImg<unsigned short> MWHSReader::getChannel(int channel)
        {
            cimg_library::CImg<unsigned short> img(98, imageVector.size() * 6, 1, 1);

            int line = 0;
            int y = 0;
            int last = 1;

            // Reconstitute the image. Works "OK", not perfect...
            for (int cnt = 0; cnt < (int)imageVector.size(); cnt++)
            {
                // Count 0s on the side
                int zeros = 0;
                for (int i = 0; i < 31; i++)
                {
                    if (imageVector[imageVector.size() - cnt].channels[channel][i * 99] == 0)
                        zeros++;
                }

                if (zeros > 10)
                    continue;

                int differences = 0;
                int lastMarker = imageVector[imageVector.size() - cnt].channels[channel][0 * 99 + 98];
                for (int i = 1; i < 31; i++)
                {
                    if (imageVector[imageVector.size() - cnt].channels[channel][i * 99 + 98] != lastMarker)
                        differences++;
                    lastMarker = imageVector[imageVector.size() - cnt].channels[channel][i * 99 + 98];
                }

                if (differences == 0 && last != 0)
                {
                    imageVector[imageVector.size() - cnt].lastMkMatch = imageVector[imageVector.size() - (cnt + 1)].lastMkMatch + 6;

                    if (imageVector[imageVector.size() - cnt].lastMkMatch > 31)
                        imageVector[imageVector.size() - cnt].lastMkMatch -= 32;
                }
                else if (differences == 0 && last == 0)
                {
                    continue;
                }

                last = differences;

                for (int i = 0; i < 6; i++)
                {
                    std::memcpy(&img.data()[line * 98], &imageVector[imageVector.size() - cnt].channels[channel][(imageVector[imageVector.size() - cnt].lastMkMatch - i) * 99], 2 * 98);
                    line++;
                }
            }

            img.normalize(0, 65535);
            img.equalize(1000);

            img.crop(0, 0, 98, line);

            img.mirror('x');

            return img;
        }
    }
}