#include "iasi_imaging_reader.h"
#include "utils.h"
#include <iostream>

IASIIMGReader::IASIIMGReader()
{
    ir_channel = new unsigned short[10000 * 64 * 30];
    lines = 0;
}

void IASIIMGReader::work(libccsds::CCSDSPacket &packet)
{
    if (packet.payload.size() < 6196)
        return;

    int counter = packet.payload[16];

    if (counter <= 30)
    {
        std::vector<uint16_t> values = repackBits(&packet.payload.data()[50], 12, 0, 6196 - 50);

        for (int i = 0; i < 64; i++)
        {
            for (int y = 0; y < 64; y++)
            {
                if (y >= 48)
                    ir_channel[(lines * 64 + i) * (30 * 64) + ((counter - 1) * 64) + (y)] = (values[y * 64 + i] + 10) << 3;
                else
                    ir_channel[(lines * 64 + i) * (30 * 64) + ((counter - 1) * 64) + (y)] = values[y * 64 + i] << 3;
            }
        }
    }

    // Frame counter
    if (counter == 36)
        lines++;
}

cimg_library::CImg<unsigned short> IASIIMGReader::getIRChannel()
{
    cimg_library::CImg<unsigned short> img = cimg_library::CImg<unsigned short>(ir_channel, 30 * 64, lines * 64);
    img.normalize(0, 65535);
    img.equalize(1000);
    img.mirror('x');
    return img;
}