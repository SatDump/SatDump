#include "mersi_250m_reader.h"
#include <cmath>

namespace fengyun3
{
    namespace mersill
    {
        MERSI250Reader::MERSI250Reader()
        {
            imageBuffer.create(20000 * 6144);
            mersiLineBuffer = new unsigned short[40960];
            frames = 0;
        }

        MERSI250Reader::~MERSI250Reader()
        {
            imageBuffer.destroy();
            delete[] mersiLineBuffer;
        }

        void MERSI250Reader::pushFrame(std::vector<uint8_t> &data)
        {
            int pos = 96 + 54; // MERSI-LL Data position, found through a bit viewer

            // Convert into 12-bits values
            for (int i = 0; i < 6144; i += 2)
            {
                byteBufShift[0] = data[pos + 0] << 3 | data[pos + 1] >> 5;
                byteBufShift[1] = data[pos + 1] << 3 | data[pos + 2] >> 5;
                byteBufShift[2] = data[pos + 2] << 3 | data[pos + 3] >> 5;

                mersiLineBuffer[i] = byteBufShift[0] << 4 | byteBufShift[1] >> 4;
                mersiLineBuffer[i + 1] = (byteBufShift[1] % (int)pow(2, 4)) << 8 | byteBufShift[2];
                pos += 3;
            }

            // Load into our image buffer
            for (int i = 0; i < 6144; i++)
            {
                uint16_t pixel = mersiLineBuffer[i];
                imageBuffer[frames * 6144 + (6143 - i)] = pixel * 15;
            }

            // Frame counter
            frames++;

            // Make sure we have enough room
            if (frames * 6144 >= (int)imageBuffer.size())
                imageBuffer.resize((frames + 10000) * 6144);
        }

        image::Image<uint16_t> MERSI250Reader::getImage()
        {
            return image::Image<uint16_t>(&imageBuffer[0], 6144, frames, 1);
        }
    } // namespace mersi1
} // namespace fengyun