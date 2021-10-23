#include "mtvza_reader.h"

namespace meteor
{
    namespace mtvza
    {
        unsigned char reverse(unsigned char b)
        {
            b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
            b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
            b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
            return b;
        }

        MTVZAReader::MTVZAReader()
        {
            for (int i = 0; i < 150; i++)
            {
                channels[i] = new unsigned short[10000 * 26 * 2];
            }
            lines = 0;
        }

        MTVZAReader::~MTVZAReader()
        {
            for (int i = 0; i < 150; i++)
            {
                delete[] channels[i];
            }
        }

        void MTVZAReader::work(uint8_t *data)
        {
            int counter = data[5];

            if (counter <= 26)
            {
                int pos = 7;
                for (int ch = 0; ch < 60; ch++)
                {
                    channels[ch][lines * 26 * 2 + (counter - 1) * 2 + 0] = data[pos + 0] << 8 | data[pos + 1];
                    channels[ch][lines * 26 * 2 + (counter - 1) * 2 + 1] = data[pos + 120 + 0] << 8 | data[pos + 120 + 1];
                    pos += 2;
                }
            }

            // Frame counter
            if (counter == 26)
                lines += 1;
        }

        cimg_library::CImg<unsigned short> MTVZAReader::getChannel(int channel)
        {
            cimg_library::CImg<unsigned short> img = cimg_library::CImg<unsigned short>(channels[channel], 26 * 2, lines);
            img.normalize(0, 65535);
            img.equalize(1000);
            //img.resize(img.width() * 2, img.height());
            //img.mirror('x');
            return img;
        }
    }
}