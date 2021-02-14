#include "mersi_1000m_reader.h"

MERSI1000Reader::MERSI1000Reader()
{
    imageBuffer = new unsigned short[20000 * 2048];
    frames = 0;
}

MERSI1000Reader::~MERSI1000Reader()
{
    delete[] imageBuffer;
}

void MERSI1000Reader::pushFrame(std::vector<uint8_t> &data)
{
    int pos = 63; // MERSI-2 Data position, found through a bit viewer

    // Convert into 12-bits values
    for (int i = 0; i < 2048; i += 2)
    {
        byteBufShift[0] = data[pos + 0] << 3 | data[pos + 1] >> 5;
        byteBufShift[1] = data[pos + 1] << 3 | data[pos + 2] >> 5;
        byteBufShift[2] = data[pos + 2] << 3 | data[pos + 3] >> 5;

        mersiLineBuffer[i] = byteBufShift[0] << 4 | byteBufShift[1] >> 4;
        mersiLineBuffer[i + 1] = (byteBufShift[1] % (int)pow(2, 4)) << 8 | byteBufShift[2];
        pos += 3;
    }

    // Load into our image buffer
    for (int i = 0; i < 2048; i++)
    {
        uint16_t pixel = mersiLineBuffer[i];
        imageBuffer[frames * 2048 + (2048 - i)] = pixel * 15;
    }

    // Frame counter
    frames++;
}

cimg_library::CImg<unsigned short> MERSI1000Reader::getImage()
{
    return cimg_library::CImg<unsigned short>(&imageBuffer[0], 2048, frames);
}
