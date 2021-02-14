#include "mhs_reader.h"

MHSReader::MHSReader()
{
    for (int i = 0; i < 5; i++)
        channels[i] = new unsigned short[10000 * 90];
    lines = 0;
}

void MHSReader::work(libccsds::CCSDSPacket &packet)
{
    if (packet.payload.size() < 1302)
        return;

    int pos = 65 - 6;

    for (int i = 0; i < 90 * 6; i++)
    {
        lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
        pos += 2;
    }

    // Plot into an image
    for (int channel = 0; channel < 5; channel++)
    {
        for (int i = 0; i < 90; i++)
        {
            channels[4 - channel][lines * 90 + 90 - i] = lineBuffer[i * 6 + channel+3];
        }
    }

    // Frame counter
    lines++;
}

cimg_library::CImg<unsigned short> MHSReader::getChannel(int channel)
{
    cimg_library::CImg<unsigned short> img = cimg_library::CImg<unsigned short>(channels[channel], 90, lines);
    img.equalize(1000);
    return img;
}