#include "gome_reader.h"
#include "utils.h"
#include <iostream>

GOMEReader::GOMEReader()
{
    for (int i = 0; i < 6144; i++)
    {
        channels[i] = new unsigned short[10000 * 15];
    }
    lines = 0;
}

std::vector<uint16_t> repackBitsIASI(uint8_t *in, int dst_bits, int skip, int lengthToConvert)
{
    std::vector<uint16_t> result;
    uint16_t shifter;
    int inShiter = 0;
    for (int i = 0; i < lengthToConvert; i++)
    {
        for (int b = 7; b >= 0; b--)
        {
            if (--skip > 0)
                continue;

            bool bit = getBit<uint8_t>(in[i], b);
            shifter = (shifter << 1 | bit) % (int)pow(2, dst_bits);
            inShiter++;
            if (inShiter == dst_bits)
            {
                result.push_back(shifter);
                inShiter = 0;
            }
        }
    }

    return result;
}

struct unsigned_int16
{
    operator unsigned short(void) const
    {
        return ((a << 8) + b);
    }

    unsigned char a;
    unsigned char b;
};

struct GBand
{
    unsigned_int16 ind;
    unsigned_int16 data[1024];
};

// Values extracted from the packet according to the "rough" documenation in the Metopizer
int band_channels[6] = {0, 0, 1, 1, 2, 3};
int band_starts[6] = {0, 659, 0, 71, 0, 0};
int band_ends[6] = {658, 1023, 70, 1023, 1023, 1023};

void GOMEReader::work(libccsds::CCSDSPacket &packet)
{
    if (packet.payload.size() < 18732)
        return;

    unsigned_int16 *header = (unsigned_int16 *)&packet.payload[14];

    int counter = header[6];

    GBand bands[2][4];
    std::memcpy(&bands[0][0], &header[478 + 680], sizeof(bands));

    for (int band = 0; band < 6; band++)
    {
        for (int channel = 0; channel < 1024; channel++)
        {
            if ((header[17] >> (10 - band * 2)) & 3)
            {
                int val = bands[0][band_channels[band]].data[band_starts[band] + channel];

                channels[band * 1024 + channel][lines * 15 + counter] = val;
            }

            if ((header[18] >> (5 - band)) & 1)
            {
                int val = bands[1][band_channels[band]].data[band_starts[band] + channel];

                channels[band * 1024 + channel][lines * 15 + counter] = val;
            }
        }
    }

    if (counter == 15)
    {
        lines++;
    }
}

cimg_library::CImg<unsigned short> GOMEReader::getChannel(int channel)
{
    cimg_library::CImg<unsigned short> img = cimg_library::CImg<unsigned short>(channels[channel], 15, lines);
    img.normalize(0, 65535);
    img.equalize(1000);
    img.mirror('x');
    img.resize(30, lines);
    return img;
}