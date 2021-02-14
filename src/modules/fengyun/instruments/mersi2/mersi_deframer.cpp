#include "mersi_deframer.h"

#include <math.h>
#include <iostream>
#include <bitset>

#define ASM_SYNC 0b0111111111111000000000000100

// Returns the asked bit!
template <typename T>
inline bool getBit(T &data, int &bit)
{
    return (data >> bit) & 1;
}

MersiDeframer::MersiDeframer()
{
    // Default values
    writeFrame = false;
    wroteBits = 0;
    outputBits = 0;
}

// Write a single bit into the frame
void MersiDeframer::pushBit(uint8_t bit)
{
    byteBuffer = (byteBuffer << 1) | bit;
    wroteBits++;
    if (wroteBits == 8)
    {
        frameBuffer.push_back(byteBuffer);
        wroteBits = 0;
    }
}

std::vector<std::vector<uint8_t>> MersiDeframer::work(std::vector<uint8_t> &data)
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

            shifter = ((shifter << 1 | bit) % (int)pow(2, 28));

            // Writing a frame!
            if (writeFrame)
            {
                // First run : push header
                if (outputBits == 0)
                {
                    uint32_t syncAsm = ASM_SYNC;
                    for (int y = 28; y >= 0; y--)
                    {
                        pushBit(getBit<uint32_t>(syncAsm, y));
                        outputBits++;
                    }
                }

                // New ASM, ABORT! and process the new one
                if (shifter == ASM_SYNC)
                {
                    // Fill up what we're missing
                    for (int b = 0; b < currentFrameSize - outputBits; b++)
                        pushBit(0);

                    writeFrame = false;
                    wroteBits = 0;
                    outputBits = 0;
                    framesOut.push_back(frameBuffer);
                    frameBuffer.clear();

                    writeFrame = true;

                    continue;
                }

                // Push current bit
                pushBit(bit);
                outputBits++;

                if (outputBits == 80)
                {
                    int marker = (frameBuffer[3] % (int)pow(2, 3)) << 7 | frameBuffer[4] >> 1;
                    //std::cout << marker << std::endl;
                    if (marker >= 240)
                    {
                        currentFrameSize = 25128;
                    }
                    else if (marker < 240)
                    {
                        currentFrameSize = 98856;
                    }
                }

                // Once we wrote a frame, exit!
                if (outputBits == currentFrameSize)
                {
                    writeFrame = false;
                    wroteBits = 0;
                    outputBits = 0;
                    framesOut.push_back(frameBuffer);
                    frameBuffer.clear();
                }

                continue;
            }

            //std::cout << std::bitset<32>(shifter) << " " << std::bitset<28>(ASM_SYNC) << std::endl;

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
