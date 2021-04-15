#include "infrared1_reader.h"

#define WIDTH 5206
#define HEIGHT (1354 * 2)

namespace goes_gvar
{
    InfraredReader1::InfraredReader1()
    {
        imageBuffer1 = new unsigned short[HEIGHT * WIDTH];
        imageBuffer2 = new unsigned short[HEIGHT * WIDTH];
        imageLineBuffer = new unsigned short[23860];
        goodLines = new bool[HEIGHT];
    }

    InfraredReader1::~InfraredReader1()
    {
        // delete[] imageBuffer1;
        // delete[] imageBuffer2;
        // delete[] imageLineBuffer;
        // delete[] goodLines;
    }

    void InfraredReader1::startNewFullDisk()
    {
        std::fill(&imageBuffer1[0], &imageBuffer1[HEIGHT * WIDTH], 0);
        std::fill(&imageBuffer2[0], &imageBuffer2[HEIGHT * WIDTH], 0);
        std::fill(&goodLines[0], &goodLines[HEIGHT], false);
    }

    void InfraredReader1::pushFrame(uint8_t *data, int counter)
    {
        // Get the current mode. Shifted?
        bool status = data[8 + 30 + 3] >> 4 & 1;

        // Offset to start reading from
        int pos = 0;
        int posb = 4;

        // Convert to 10 bits values
        for (int i = 0; i < 23860; i += 4)
        {
            byteBufShift[0] = data[pos + 0] << posb | data[pos + 1] >> (8 - posb);
            byteBufShift[1] = data[pos + 1] << posb | data[pos + 2] >> (8 - posb);
            byteBufShift[2] = data[pos + 2] << posb | data[pos + 3] >> (8 - posb);
            byteBufShift[3] = data[pos + 3] << posb | data[pos + 4] >> (8 - posb);
            byteBufShift[4] = data[pos + 4] << posb | data[pos + 5] >> (8 - posb);

            imageLineBuffer[i] = (byteBufShift[0] << 2) | (byteBufShift[1] >> 6);
            imageLineBuffer[i + 1] = ((byteBufShift[1] % 64) << 4) | (byteBufShift[2] >> 4);
            imageLineBuffer[i + 2] = ((byteBufShift[2] % 16) << 6) | (byteBufShift[3] >> 2);
            imageLineBuffer[i + 3] = ((byteBufShift[3] % 4) << 8) | byteBufShift[4];
            pos += 5;
        }

        // Deinterleave channel 1 and load into our image buffer
        for (int i = 0; i < WIDTH; i++)
        {
            uint16_t pixel1 = imageLineBuffer[94 + i];
            uint16_t pixel2 = imageLineBuffer[(status ? 5316 : 5320) + i];
            imageBuffer1[((counter * 2 + 0) * WIDTH) + i] = pixel1 << 6;
            imageBuffer1[((counter * 2 + 1) * WIDTH) + i] = pixel2 << 6;
        }

        for (int i = 0; i < WIDTH; i++)
        {
            uint16_t pixel1 = imageLineBuffer[10538 + i];
            uint16_t pixel2 = imageLineBuffer[(status ? 15760 : 15764) + i];
            imageBuffer2[((counter * 2 + 0) * WIDTH) + i] = pixel1 << 6;
            imageBuffer2[((counter * 2 + 1) * WIDTH) + i] = (i >= (WIDTH - 44) && status) ? pixel1 << 6 : pixel2 << 6; // Some data is missing...
        }

        goodLines[counter * 2 + 0] = true;
        goodLines[counter * 2 + 1] = true;
    }

    cimg_library::CImg<unsigned short> InfraredReader1::getImage1()
    {
        // Fill missing lines by averaging above and below line
        for (int y = 1; y < HEIGHT - 2; y++)
        {
            bool &current = goodLines[y];
            if (!current)
            {
                for (int i = 0; i < WIDTH; i++)
                {
                    unsigned short &above = imageBuffer1[((y - 1) * WIDTH) + i];
                    unsigned short &below = imageBuffer1[((y + 2) * WIDTH) + i];
                    imageBuffer1[(y * WIDTH) + i] = (above + below) / 2;
                }
            }
        }

        return cimg_library::CImg<unsigned short>(&imageBuffer1[0], WIDTH, HEIGHT);
    }

    cimg_library::CImg<unsigned short> InfraredReader1::getImage2()
    {
        // Fill missing lines by averaging above and below line
        for (int y = 1; y < HEIGHT - 2; y++)
        {
            bool &current = goodLines[y];
            if (!current)
            {
                for (int i = 0; i < WIDTH; i++)
                {
                    unsigned short &above = imageBuffer2[((y - 1) * WIDTH) + i];
                    unsigned short &below = imageBuffer2[((y + 2) * WIDTH) + i];
                    imageBuffer2[(y * WIDTH) + i] = (above + below) / 2;
                }
            }
        }

        return cimg_library::CImg<unsigned short>(&imageBuffer2[0], WIDTH, HEIGHT);
    }
}