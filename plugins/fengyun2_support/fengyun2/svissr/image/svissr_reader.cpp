#include "svissr_reader.h"
#include <cmath>

#define MAX_HEIGHT 2501
#define WIDTH_IR 2291
#define WIDTH_VIS 2290 * 4

namespace fengyun_svissr
{
    SVISSRReader::SVISSRReader()
    {
        imageBufferIR1 = new unsigned short[MAX_HEIGHT * WIDTH_IR];
        imageBufferIR2 = new unsigned short[MAX_HEIGHT * WIDTH_IR];
        imageBufferIR3 = new unsigned short[MAX_HEIGHT * WIDTH_IR];
        imageBufferIR4 = new unsigned short[MAX_HEIGHT * WIDTH_IR];
        imageBufferVIS = new unsigned short[MAX_HEIGHT * 4 * WIDTH_VIS];
        imageLineBuffer = new unsigned short[40000];
        goodLines = new bool[MAX_HEIGHT];
    }

    SVISSRReader::~SVISSRReader()
    {
        delete[] imageBufferIR1;
        delete[] imageBufferIR2;
        delete[] imageBufferIR3;
        delete[] imageBufferIR4;
        delete[] imageBufferVIS;
        delete[] imageLineBuffer;
        delete[] goodLines;
    }

    void SVISSRReader::reset()
    {
        std::fill(&imageBufferIR1[0], &imageBufferIR1[MAX_HEIGHT * WIDTH_IR], 0);
        std::fill(&imageBufferIR2[0], &imageBufferIR2[MAX_HEIGHT * WIDTH_IR], 0);
        std::fill(&imageBufferIR3[0], &imageBufferIR3[MAX_HEIGHT * WIDTH_IR], 0);
        std::fill(&imageBufferIR4[0], &imageBufferIR4[MAX_HEIGHT * WIDTH_IR], 0);
        std::fill(&imageBufferVIS[0], &imageBufferVIS[MAX_HEIGHT * 4 * WIDTH_VIS], 0);
        std::fill(&goodLines[0], &goodLines[MAX_HEIGHT], false);
    }

    void SVISSRReader::pushFrame(uint8_t *data)
    {
        int counter = (data[67]) << 8 | (data[68]); // Decode line counter

        // Safeguard
        if (counter >= MAX_HEIGHT)
            return;

        // Decode IR Channel 1
        {
            int pos = 20408 / 8 + 2;
            for (int i = 0; i < WIDTH_IR; i++)
            {
                uint16_t pixel = pow(2, 8) - data[pos + i];
                imageBufferIR1[counter * WIDTH_IR + i] = (pixel << 2) * 60;
            }
        }

        // Decode IR Channel 2
        {
            int pos = (20408 / 8) * 2 + 2;
            for (int i = 0; i < WIDTH_IR; i++)
            {
                uint16_t pixel = pow(2, 8) - data[pos + i];
                imageBufferIR2[counter * WIDTH_IR + i] = (pixel << 2) * 60;
            }
        }

        // Decode IR Channel 3
        {
            int pos = (20408 / 8) * 3 + 2;
            for (int i = 0; i < WIDTH_IR; i++)
            {
                uint16_t pixel = pow(2, 8) - data[pos + i];
                imageBufferIR3[counter * WIDTH_IR + i] = (pixel << 2) * 60;
            }
        }

        // Decode IR Channel 4
        {
            // Offset to start reading from
            int pos = ((20408 * 4 + 57060 * 4 + 6662 * 3) / 8) + 2;
            int posb = 2;

            // Convert to 10 bits values
            for (int i = 0; i < WIDTH_VIS; i += 4)
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

            // Deinterleave and load into our image buffer
            for (int i = 0; i < WIDTH_IR; i++)
            {
                uint16_t pixel = pow(2, 10) - imageLineBuffer[i];
                imageBufferIR4[counter * WIDTH_IR + i] = pixel * 60;
            }
        }

        // Decode VIS
        {
            for (int run = 0; run < 4; run++)
            {
                // Offset to start reading from
                int pos = ((20408 * 4 + 57060 * run) / 8) + (run % 2 == 0 ? -1 : 0) + 3;
                int posb = run % 2 == 0 ? 8 : 4;

                // Convert to 10 bits values
                for (int i = 0; i < WIDTH_VIS; i += 4)
                {
                    byteBufShift[0] = data[pos + 0] << posb | data[pos + 1] >> (8 - posb);
                    byteBufShift[1] = data[pos + 1] << posb | data[pos + 2] >> (8 - posb);
                    byteBufShift[2] = data[pos + 2] << posb | data[pos + 3] >> (8 - posb);

                    imageLineBuffer[i] = byteBufShift[0] >> 2;
                    imageLineBuffer[i + 1] = (byteBufShift[0] % 4) << 4 | byteBufShift[1] >> 4;
                    imageLineBuffer[i + 2] = (byteBufShift[1] % 16) << 2 | byteBufShift[2] >> 6;
                    imageLineBuffer[i + 3] = byteBufShift[2] % (int)pow(2, 6);
                    pos += 3;
                }

                // Deinterleave and load into our image buffer
                for (int i = 0; i < WIDTH_VIS; i++)
                {
                    uint16_t pixel = /*pow(2, 6) -*/ imageLineBuffer[i];
                    imageBufferVIS[(counter * 4 + run) * WIDTH_VIS + i] = (pixel << 4) * 60;
                }
            }
        }

        goodLines[counter] = true;
    }

    image::Image<uint16_t> SVISSRReader::getImageIR1()
    {
        // Fill missing lines by averaging above and below line
        for (int y = 1; y < MAX_HEIGHT - 1; y++)
        {
            bool &current = goodLines[y];
            if (!current)
            {
                for (int i = 0; i < WIDTH_IR; i++)
                {
                    unsigned short &above = imageBufferIR1[((y - 1) * WIDTH_IR) + i];
                    unsigned short &below = imageBufferIR1[((y + 1) * WIDTH_IR) + i];
                    imageBufferIR1[(y * WIDTH_IR) + i] = (above + below) / 2;
                }
            }
        }

        return image::Image<uint16_t>(&imageBufferIR1[0], WIDTH_IR, MAX_HEIGHT, 1);
    }

    image::Image<uint16_t> SVISSRReader::getImageIR2()
    {
        // Fill missing lines by averaging above and below line
        for (int y = 1; y < MAX_HEIGHT - 1; y++)
        {
            bool &current = goodLines[y];
            if (!current)
            {
                for (int i = 0; i < WIDTH_IR; i++)
                {
                    unsigned short &above = imageBufferIR2[((y - 1) * WIDTH_IR) + i];
                    unsigned short &below = imageBufferIR2[((y + 1) * WIDTH_IR) + i];
                    imageBufferIR2[(y * WIDTH_IR) + i] = (above + below) / 2;
                }
            }
        }

        return image::Image<uint16_t>(&imageBufferIR2[0], WIDTH_IR, MAX_HEIGHT, 1);
    }

    image::Image<uint16_t> SVISSRReader::getImageIR3()
    {
        // Fill missing lines by averaging above and below line
        for (int y = 1; y < MAX_HEIGHT - 1; y++)
        {
            bool &current = goodLines[y];
            if (!current)
            {
                for (int i = 0; i < WIDTH_IR; i++)
                {
                    unsigned short &above = imageBufferIR3[((y - 1) * WIDTH_IR) + i];
                    unsigned short &below = imageBufferIR3[((y + 1) * WIDTH_IR) + i];
                    imageBufferIR3[(y * WIDTH_IR) + i] = (above + below) / 2;
                }
            }
        }

        return image::Image<uint16_t>(&imageBufferIR3[0], WIDTH_IR, MAX_HEIGHT, 1);
    }

    image::Image<uint16_t> SVISSRReader::getImageIR4()
    {
        // Fill missing lines by averaging above and below line
        for (int y = 1; y < MAX_HEIGHT - 1; y++)
        {
            bool &current = goodLines[y];
            if (!current)
            {
                for (int i = 0; i < WIDTH_IR; i++)
                {
                    unsigned short &above = imageBufferIR4[((y - 1) * WIDTH_IR) + i];
                    unsigned short &below = imageBufferIR4[((y + 1) * WIDTH_IR) + i];
                    imageBufferIR4[(y * WIDTH_IR) + i] = (above + below) / 2;
                }
            }
        }

        return image::Image<uint16_t>(&imageBufferIR4[0], WIDTH_IR, MAX_HEIGHT, 1);
    }

    image::Image<uint16_t> SVISSRReader::getImageVIS()
    {
        // Fill missing lines by averaging above and below line
        for (int y = 1; y < MAX_HEIGHT - 1; y++)
        {
            bool &current = goodLines[y];
            if (!current)
            {
                for (int i = 0; i < WIDTH_VIS; i++)
                {
                    unsigned short &above = imageBufferVIS[((y * 4 - 1) * WIDTH_VIS) + i];
                    unsigned short &below = imageBufferVIS[((y * 4 + 4) * WIDTH_VIS) + i];
                    imageBufferVIS[((y * 4 + 0) * WIDTH_VIS) + i] = (above + below) / 2;
                    imageBufferVIS[((y * 4 + 1) * WIDTH_VIS) + i] = (above + below) / 2;
                    imageBufferVIS[((y * 4 + 2) * WIDTH_VIS) + i] = (above + below) / 2;
                    imageBufferVIS[((y * 4 + 3) * WIDTH_VIS) + i] = (above + below) / 2;
                }
            }
        }

        return image::Image<uint16_t>(&imageBufferVIS[0], WIDTH_VIS, MAX_HEIGHT * 4, 1);
    }
}