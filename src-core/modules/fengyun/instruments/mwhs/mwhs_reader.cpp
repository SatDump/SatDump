#include "mwhs_reader.h"

#include <iostream>

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

            //int marker2 = (packet.payload[3] >> 5) & 1;

            //if (marker2 != 0)
            //    return;

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
                imageVector[imageVector.size() - 1].channels[0][counter * 98 + (marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 0];
                imageVector[imageVector.size() - 1].channels[1][counter * 98 + (marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 1];
                imageVector[imageVector.size() - 1].channels[2][counter * 98 + (marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 2];
                imageVector[imageVector.size() - 1].channels[3][counter * 98 + (marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 3];
                imageVector[imageVector.size() - 1].channels[4][counter * 98 + (marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 4];
                imageVector[imageVector.size() - 1].channels[5][counter * 98 + (marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 5];
            }

            //if (mk == 1)
            //    imageVector[imageVector.size() - 1].channels[0][counter * 98 + 0] = 65535;
            //else
            //    imageVector[imageVector.size() - 1].channels[0][counter * 98 + 0] = 0;

            if (counter == 31 && marker == 0)
            {
                imageVector.push_back(MWHSImage());
                lines += 6;
            }

            lines++;
        }

        cimg_library::CImg<unsigned short> MWHSReader::getChannel(int channel)
        {
            //for (int i = 0; i < imageVector.size(); i++)
            //{
            //    std::cout << "Frame " << (i + 1) << std::endl;
            //    cimg_library::CImg<unsigned short> img(imageVector[i].channels[0], 98, 32);
            //    img.resize(img.width() * 20, img.height() * 20);
            //    img.save_png(std::string("MWHS_FR-" + std::to_string(i + 1) + ".png").c_str());
            //}

            cimg_library::CImg<unsigned short> img(98, imageVector.size() * 6, 1, 1);

            int line = 0;

            // Reconstitute the image. Works "OK", not perfect...
            for (int cnt = 0; cnt < imageVector.size(); cnt++)
            {
                // Count 0s on the side
                //int zeros = 0;
                //for (int i = 0; i < 31; i++)
                //{
                //    if (imageVector[imageVector.size() - cnt].channels[channel][i * 98] == 0)
                //        zeros++;
                //}

                //if (zeros > 15)
                //    continue;

                for (int i = 0; i < 6; i++)
                {
                    std::memcpy(&img.data()[line * 98], &imageVector[imageVector.size() - cnt].channels[channel][(imageVector[imageVector.size() - cnt].lastMkMatch - i) * 98], 2 * 98);
                    line++;
                }
            }

            img.normalize(0, 65535);
            img.equalize(1000);
            img.mirror('x');

            return img;
        }
    }
}