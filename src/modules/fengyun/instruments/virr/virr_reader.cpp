#include "virr_reader.h"

VIRRReader::VIRRReader()
{
    for (int i = 0; i < 10; i++)
        channels[i] = new unsigned short[10000 * 2048];

    lines = 0;
}

void VIRRReader::work(std::vector<uint8_t> &packet)
{
    if (packet.size() < 12960)
        return;

    int pos = 424; // VIRR Data position, found through a bit viewer

    // Convert into 10-bits values
    for (int i = 0; i < 20480; i += 4)
    {
        virrBuffer[i] = (packet[pos + 0] << 2) | (packet[pos + 1] >> 6);
        virrBuffer[i + 1] = ((packet[pos + 1] % 64) << 4) | (packet[pos + 2] >> 4);
        virrBuffer[i + 2] = ((packet[pos + 2] % 16) << 6) | (packet[pos + 3] >> 2);
        virrBuffer[i + 3] = ((packet[pos + 3] % 4) << 8) | packet[pos + 4];
        pos += 5;
    }

    for (int channel = 0; channel < 10; channel++)
    {
        for (int i = 0; i < 2048; i++)
        {
            uint16_t pixel = virrBuffer[channel + i * 10];
            channels[channel][lines * 2048 + i] = pixel * 60;
        }
    }

    // Frame counter
    lines++;
}

cimg_library::CImg<unsigned short> VIRRReader::getChannel(int channel)
{
    return cimg_library::CImg<unsigned short>(channels[channel], 2048, lines);
}