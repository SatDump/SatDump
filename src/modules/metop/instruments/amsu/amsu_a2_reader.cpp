#include "amsu_a2_reader.h"

AMSUA2Reader::AMSUA2Reader()
{
    for (int i = 0; i < 2; i++)
        channels[i] = new unsigned short[10000 * 30];
    lines = 0;
}

void AMSUA2Reader::work(libccsds::CCSDSPacket &packet)
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
            channels[channel][lines * 30 + 30 - i] = pixel;
        }
    }

    // Frame counter
    lines++;
}

cimg_library::CImg<unsigned short> AMSUA2Reader::getChannel(int channel)
{
    cimg_library::CImg<unsigned short> img = cimg_library::CImg<unsigned short>(channels[channel], 30, lines);
    img.equalize(1000);
    return img;
}