#include "xeuvi_deframer.h"

#include <math.h>
//#include <iostream>
//#include <bitset>

#define ASM_SYNC 0x55aa55aa55aa

// Returns the asked bit!
template <typename T>
inline bool getBit(T &data, int &bit)
{
    return (data >> bit) & 1;
}

namespace fengyun3
{
    namespace xeuvi
    {
        XEUVIDeframer::XEUVIDeframer()
        {
            // Default values
            writeFrame = false;
            wroteBits = 0;
            outputBits = 0;
        }

        // Write a single bit into the frame
        void XEUVIDeframer::pushBit(uint8_t bit)
        {
            byteBuffer = (byteBuffer << 1) | bit;
            wroteBits++;
            if (wroteBits == 8)
            {
                frameBuffer.push_back(byteBuffer);
                wroteBits = 0;
            }
        }

        std::vector<std::vector<uint8_t>> XEUVIDeframer::work(uint8_t *input, int size)
        {
            // Output buffer
            std::vector<std::vector<uint8_t>> framesOut;

            // Loop in all bytes
            for (int byteInBuf = 0; byteInBuf < size; byteInBuf++)
            {
                // Loop in all bits!
                for (int i = 7; i >= 0; i--)
                {
                    // Get a bit, push it
                    uint8_t bit = getBit<uint8_t>(input[byteInBuf], i);

                    shifter = ((shifter << 1 | bit) % 281474976710656); // 2^40

                    // Writing a frame!
                    if (writeFrame)
                    {
                        // First run : push header
                        if (outputBits == 0)
                        {
                            uint64_t syncAsm = ASM_SYNC;
                            for (int y = 48 - 1; y >= 0; y--)
                            {
                                pushBit(getBit<uint64_t>(syncAsm, y));
                                outputBits++;
                            }
                        }

                        // New ASM, ABORT! and process the new one
                        if (shifter == ASM_SYNC)
                        {
                            // Clear ASM out
                            frameBuffer[frameBuffer.size() - 1 - 0] = 0;
                            frameBuffer[frameBuffer.size() - 1 - 1] = 0;
                            frameBuffer[frameBuffer.size() - 1 - 2] = 0;
                            frameBuffer[frameBuffer.size() - 1 - 3] = 0;
                            frameBuffer[frameBuffer.size() - 1 - 4] = 0;
                            frameBuffer[frameBuffer.size() - 1 - 5] = 0;
                            frameBuffer[frameBuffer.size() - 1 - 6] = 0;

                            // Fill up what we're missing
                            for (int b = 0; b < 519328 - outputBits; b++)
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
                        if (outputBits == 519328)
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
    } // namespace virr
} // namespace fengyun