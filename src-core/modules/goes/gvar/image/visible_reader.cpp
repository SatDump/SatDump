#include "visible_reader.h"

#define WIDTH 20944 //30000//20824
#define HEIGHT (1354 * 8)

namespace goes
{
    namespace gvar
    {
        VisibleReader::VisibleReader()
        {
            imageBuffer = new unsigned short[HEIGHT * WIDTH];
            imageLineBuffer = new unsigned short[WIDTH + 10];
            goodLines = new bool[HEIGHT];
        }

        VisibleReader::~VisibleReader()
        {
            delete[] imageBuffer;
            delete[] imageLineBuffer;
            delete[] goodLines;
        }

        void VisibleReader::startNewFullDisk()
        {
            std::fill(&imageBuffer[0], &imageBuffer[HEIGHT * WIDTH], 0);
            std::fill(&goodLines[0], &goodLines[HEIGHT], false);
        }

        void VisibleReader::pushFrame(uint8_t *data, int block, int counter)
        {
            // Offset to start reading from
            int pos = 116;
            int posb = 6;

            // Convert to 10 bits values
            for (int i = 0; i < WIDTH + 10; i += 4)
            {
                byteBufShift[0] = data[pos + 0] << posb | data[pos + 1] >> (8 - posb);
                byteBufShift[1] = data[pos + 1] << posb | data[pos + 2] >> (8 - posb);
                byteBufShift[2] = data[pos + 2] << posb | data[pos + 3] >> (8 - posb);
                byteBufShift[3] = data[pos + 3] << posb | data[pos + 4] >> (8 - posb);
                byteBufShift[4] = data[pos + 4] << posb | data[pos + 5] >> (8 - posb);

                //data_out.write((char *)&byteBufShift[0], 4);

                imageLineBuffer[i] = (byteBufShift[0] << 2) | (byteBufShift[1] >> 6);
                imageLineBuffer[i + 1] = ((byteBufShift[1] % 64) << 4) | (byteBufShift[2] >> 4);
                imageLineBuffer[i + 2] = ((byteBufShift[2] % 16) << 6) | (byteBufShift[3] >> 2);
                imageLineBuffer[i + 3] = ((byteBufShift[3] % 4) << 8) | byteBufShift[4];
                pos += 5;
            }

            // Deinterleave and load into our image buffer
            for (int i = 0; i < WIDTH; i++)
            {
                uint16_t pixel = imageLineBuffer[i + 1];
                imageBuffer[((counter * 8 + (block - 3)) * WIDTH) + i] = pixel << 6;
                goodLines[counter * 8 + (block - 3)] = true;
            }
        }

        image::Image<uint16_t> VisibleReader::getImage()
        {
            // Fill missing lines by averaging above and below line
            for (int y = 1; y < HEIGHT - 1; y++)
            {
                bool &current = goodLines[y];
                if (!current)
                {
                    for (int i = 0; i < WIDTH; i++)
                    {
                        unsigned short &above = imageBuffer[((y - 1) * WIDTH) + i];
                        unsigned short &below = imageBuffer[((y + 1) * WIDTH) + i];
                        imageBuffer[(y * WIDTH) + i] = (above + below) / 2;
                    }
                }
            }

            return image::Image<uint16_t>(&imageBuffer[0], WIDTH, HEIGHT, 1);
        }
    }
}