#include "oceansat2_deframer.h"

#include <math.h>

#define ASM_SYNC 0b0000000111111011111000010011001101

// Returns the asked bit!
template <typename T>
inline bool getBit(T &data, int &bit)
{
    return (data >> bit) & 1;
}

namespace oceansat
{
    Oceansat2Deframer::Oceansat2Deframer()
    {
        // Default values
        writeFrame = false;
        wroteBits = 0;
        outputBits = 0;
        shifter = 0;
    }

    // Write a single bit into the frame
    void Oceansat2Deframer::pushBit(uint8_t bit)
    {
        byteBuffer = (byteBuffer << 1) | bit;
        wroteBits++;
        if (wroteBits == 8)
        {
            frameBuffer.push_back(byteBuffer);
            wroteBits = 0;
        }
    }

    std::vector<std::vector<uint8_t>> Oceansat2Deframer::work(uint8_t *data, int length)
    {
        // Output buffer
        std::vector<std::vector<uint8_t>> framesOut;

        // Loop in all bytes
        for (int ii = 0; ii < length; ii++)
        {
            uint8_t byte = data[ii];

            // Loop in all bits!
            for (int i = 7; i >= 0; i--)
            {
                // Get a bit, push it
                uint8_t bit = getBit<uint8_t>(byte, i);

                shifter = (shifter << 1 | bit) % 17179869184; // 2^34

                // Writing a frame!
                if (writeFrame)
                {
                    // First run : push header
                    if (outputBits == 0)
                    {
                        uint64_t syncAsm = ASM_SYNC;
                        for (int y = 34 - 1; y >= 0; y--)
                        {
                            pushBit(getBit<uint64_t>(syncAsm, y));
                            outputBits++;
                        }
                    }

                    // New ASM, ABORT! and process the new one
                    if (shifter == ASM_SYNC)
                    {
                        // Fill up what we're missing
                        for (int b = 0; b < frameSize - outputBits; b++)
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
                    if (outputBits == frameSize)
                    {
                        writeFrame = false;
                        wroteBits = 0;
                        outputBits = 0;
                        framesOut.push_back(frameBuffer);
                        frameBuffer.clear();
                    }

                    continue;
                }

                // std::cout << std::bitset<64>(shifter) << " " << std::bitset<64>(ASM_SYNC) << std::endl;

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
} // namespace oceansat