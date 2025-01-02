#include "msu_vis_reader.h"

namespace elektro_arktika
{
    namespace msugs
    {
        MSUVISReader::MSUVISReader()
        {
            imageBuffer1 = new unsigned short[17200 * 6004];
            imageBuffer2 = new unsigned short[17200 * 6004];
            frames = 0;
        }

        MSUVISReader::~MSUVISReader()
        {
            delete[] imageBuffer1;
            delete[] imageBuffer2;
        }

        void MSUVISReader::pushFrame(uint8_t *data, int offset)
        {
            int counter = (data[8] << 8 | data[9]) + offset;

            if (counter >= 17200)
                return;

            // Offset to start reading from
            int pos = 5 * 38;

            // Convert to 10 bits values
            for (int i = 0; i < 12044; i += 4)
            {
                msuLineBuffer[i] = (data[pos + 0] << 2) | (data[pos + 1] >> 6);
                msuLineBuffer[i + 1] = ((data[pos + 1] % 64) << 4) | (data[pos + 2] >> 4);
                msuLineBuffer[i + 2] = ((data[pos + 2] % 16) << 6) | (data[pos + 3] >> 2);
                msuLineBuffer[i + 3] = ((data[pos + 3] % 4) << 8) | data[pos + 4];
                pos += 5;
            }

            // Deinterleave and load into our image buffer
            for (int i = 0; i < 6004; i++)
            {
                imageBuffer1[counter * 6004 + i] = msuLineBuffer[i * 2 + 0] << 6;
                imageBuffer2[counter * 6004 + i] = msuLineBuffer[i * 2 + 1] << 6;
            }

            frames++;
        }

        image::Image MSUVISReader::getImage1()
        {
            return image::Image(&imageBuffer1[0], 16, 6004, 17200, 1);
        }

        image::Image MSUVISReader::getImage2()
        {
            return image::Image(&imageBuffer2[0], 16, 6004, 17200, 1);
        }
    } // namespace msugs
} // namespace elektro_arktika