#include "mtvza_reader.h"

MTVZAReader::MTVZAReader()
{
    for (int i = 0; i < 150; i++)
    {
        channels[i] = new unsigned short[10000 * 26];
    }
    lines = 0;
}

void MTVZAReader::work(uint8_t *data)
{
    int counter = data[5];

    if (counter <= 26)
    {
        int pos = 7;
        for (int ch = 0; ch < 120; ch++)
        {
            channels[ch][lines * 26 + (counter - 1)] = data[pos + 0] << 8 | data[pos + 1];
            pos += 2;
        }
    }

    // Frame counter
    if (counter == 26)
        lines += 1;
}

cimg_library::CImg<unsigned short> MTVZAReader::getChannel(int channel)
{
    cimg_library::CImg<unsigned short> img = cimg_library::CImg<unsigned short>(channels[channel], 26, lines);
    img.normalize(0, 65535);
    img.equalize(1000);
    img.resize(img.width() * 5, img.height());
    //img.mirror('x');
    return img;
}