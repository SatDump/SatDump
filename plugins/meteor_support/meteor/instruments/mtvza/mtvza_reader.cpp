#include "mtvza_reader.h"

namespace meteor
{
    namespace mtvza
    {
        MTVZAReader::MTVZAReader()
        {
            for (int i = 0; i < 60; i++)
            {
                channels[i] = new unsigned short[2000 * 52];
            }
            lines = 0;
        }

        MTVZAReader::~MTVZAReader()
        {
            for (int i = 0; i < 60; i++)
            {
                delete[] channels[i];
            }
        }

        void MTVZAReader::work(uint8_t *data)
        {
            int counter = data[5];

            if (counter <= 26 && counter > 0)
            {
                int pos = 7;
                for (int ch = 0; ch < 60; ch++)
                {
                    channels[ch][lines * 52 + (counter - 1) * 2 + 0] = data[pos + 0] << 8 | data[pos + 1];
                    channels[ch][lines * 52 + (counter - 1) * 2 + 1] = data[pos + 120 + 0] << 8 | data[pos + 120 + 1];
                    pos += 2;
                }
            }

            // Frame counter
            if (counter == 26)
                lines += 1;
        }

        image::Image<uint16_t> MTVZAReader::getChannel(int channel)
        {
            image::Image<uint16_t> img = image::Image<uint16_t>(channels[channel], 52, lines, 1);
            img.normalize();
            img.equalize();
            //img.resize(img.width() * 2, img.height());
            //img.mirror('x');
            return img;
        }
    }
}