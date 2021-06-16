#include "erm_reader.h"

#include <iostream>

namespace fengyun
{
    namespace erm
    {
        ERMReader::ERMReader()
        {
            imageVector.push_back(ERMImage());
            lines = 4;
        }

        ERMReader::~ERMReader()
        {
        }

        void ERMReader::work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 1018)
                return;

            int counter = packet.payload[36] & 0b00011111;
            int mk = (packet.payload[36] >> 5) & 1;

            if (imageVector[imageVector.size() - 1].mk == -1)
                imageVector[imageVector.size() - 1].mk = mk;

            if (mk == imageVector[imageVector.size() - 1].mk)
            {
                imageVector[imageVector.size() - 1].lastMkMatch = counter;

                int pos = 38 + 32 * 2;

                for (int i = 0; i < 151; i++)
                {
                    imageVector[imageVector.size() - 1].imageData[counter * 151 + i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                    pos += 2;
                }
            }

            //if (mk == 1)
            //    imageVector[imageVector.size() - 1].imageData[counter * 151 + 0] = 6000;
            //else
            //    imageVector[imageVector.size() - 1].imageData[counter * 151 + 0] = 4000;

            if (counter == 31)
            {
                imageVector.push_back(ERMImage());
                lines += 4;
            }
        }

        cimg_library::CImg<unsigned short> ERMReader::getChannel()
        {
            cimg_library::CImg<unsigned short> img(151, imageVector.size() * 4, 1, 1);

            if (imageVector.size() > 2)
            {
                int line = 0;

                // Reconstitute the image. Works "OK", not perfect...
                for (int cnt = 0; cnt < (int)imageVector.size(); cnt++)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        std::memcpy(&img.data()[line * 151], &imageVector[imageVector.size() - cnt].imageData[(imageVector[imageVector.size() - cnt].lastMkMatch - i) * 151], 2 * 151);
                        line++;
                    }
                }

                img.normalize(0, 65535);
                img.equalize(1000);
                img.mirror('x');
            }

            return img;
        }
    }
}