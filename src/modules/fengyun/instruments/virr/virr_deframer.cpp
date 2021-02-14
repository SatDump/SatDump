#include "virr_deframer.h"

#include <math.h>
//#include <iostream>
//#include <bitset>

#define ASM_SYNC 0b101000010001011011111101011100011001110110000011110010010101

// Returns the asked bit!
template <typename T>
inline bool getBit(T &data, int &bit)
{
    return (data >> bit) & 1;
}

VIRRDeframer::VIRRDeframer()
{
    // Default values
    writeFrame = false;
    wroteBits = 0;
    outputBits = 0;
}

// Write a single bit into the frame
void VIRRDeframer::pushBit(uint8_t bit)
{
    byteBuffer = (byteBuffer << 1) | bit;
    wroteBits++;
    if (wroteBits == 8)
    {
        frameBuffer.push_back(byteBuffer);
        wroteBits = 0;
    }
}

std::vector<std::vector<uint8_t>> VIRRDeframer::work(std::vector<uint8_t> &data)
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

            shifter = ((shifter << 1 | bit) % 1152921504606846976); // 2^60

            // Writing a frame!
            if (writeFrame)
            {
                // First run : push header
                if (outputBits == 0)
                {
                    uint64_t syncAsm = ASM_SYNC;
                    for (int y = 60 + 1; y >= 0; y--)
                    {
                        pushBit(getBit<uint64_t>(syncAsm, y));
                        outputBits++;
                    }
                }

                // New ASM, ABORT! and process the new one
                if (shifter == ASM_SYNC)
                {
                    // Fill up what we're missing
                    for (int b = 0; b < 208400 - outputBits; b++)
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

                // Once we wrote a frame, exit!
                if (outputBits == 208400)
                {
                    writeFrame = false;
                    wroteBits = 0;
                    outputBits = 0;
                    framesOut.push_back(frameBuffer);
                    frameBuffer.clear();
                }

                continue;
            }

            //std::cout << std::bitset<60>(shifter) << " " << std::bitset<60>(ASM_SYNC) << std::endl;

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
