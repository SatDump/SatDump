#include "simpledeframer.h"

#include <math.h>

// Returns the asked bit!
template <typename T>
inline bool getBit(T &data, int &bit)
{
    return (data >> bit) & 1;
}

template <typename SYNC_T, int SYNC_SIZE, int FRAME_SIZE, SYNC_T ASM_SYNC>
SimpleDeframer<SYNC_T, SYNC_SIZE, FRAME_SIZE, ASM_SYNC>::SimpleDeframer()
{
    // Default values
    writeFrame = false;
    wroteBits = 0;
    outputBits = 0;
}

// Write a single bit into the frame
template <typename SYNC_T, int SYNC_SIZE, int FRAME_SIZE, SYNC_T ASM_SYNC>
void SimpleDeframer<SYNC_T, SYNC_SIZE, FRAME_SIZE, ASM_SYNC>::pushBit(uint8_t bit)
{
    byteBuffer = (byteBuffer << 1) | bit;
    wroteBits++;
    if (wroteBits == 8)
    {
        frameBuffer.push_back(byteBuffer);
        wroteBits = 0;
    }
}

template <typename SYNC_T, int SYNC_SIZE, int FRAME_SIZE, SYNC_T ASM_SYNC>
std::vector<std::vector<uint8_t>> SimpleDeframer<SYNC_T, SYNC_SIZE, FRAME_SIZE, ASM_SYNC>::work(std::vector<uint8_t> &data)
{
    // Output buffer
    std::vector<std::vector<uint8_t>> framesOut;

    // Loop in all bytes
    for (uint8_t &byte : data)
    {
        // Loop in all bits!
        for (int i = 7; i >= 0; i--)
        {
            // Get a bit, push it
            uint8_t bit = getBit<uint8_t>(byte, i);

            if (sizeof(SYNC_T) * 8 != SYNC_SIZE)
                shifter = ((shifter << 1) % (long)pow(2, SYNC_SIZE)) | bit;
            else
                shifter = (shifter << 1) | bit;

            // Writing a frame!
            if (writeFrame)
            {
                // First run : push header
                if (outputBits == 0)
                {
                    SYNC_T syncAsm = ASM_SYNC;
                    for (int y = SYNC_SIZE - 1; y >= 0; y--)
                    {
                        pushBit(getBit<SYNC_T>(syncAsm, y));
                        outputBits++;
                    }
                }

                // Push current bit
                pushBit(bit);
                outputBits++;

                // Once we wrote a frame, exit!
                if (outputBits == FRAME_SIZE)
                {
                    writeFrame = false;
                    wroteBits = 0;
                    outputBits = 0;
                    framesOut.push_back(frameBuffer);
                    frameBuffer.clear();
                }

                continue;
            }

            // Otherwise search for markers
            if (shifter == ASM_SYNC)
            {
                writeFrame = true;
            }
        }
    }

    // Output what we found if anything
    return framesOut;
}

template class SimpleDeframer<uint32_t, 24, 312 * 8, 0xFFFFFF>;
template class SimpleDeframer<uint32_t, 24, 1240 * 8, 0xFFFFFF>;
