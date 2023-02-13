#include "svissr_deframer.h"

#include <math.h>

// Returns the asked bit!
template <typename T>
inline bool getBit(T &data, int &bit)
{
    return (data >> bit) & 1;
}

#define MAX_ERROR 7

namespace fengyun_svissr
{
    // Compare 2 32-bits values bit per bit
    int checkSyncMarker(uint64_t marker, uint64_t &totest)
    {
        int errors = 0;
        for (int i = 63; i >= 0; i--)
        {
            bool markerBit, testBit;
            markerBit = getBit<uint64_t>(marker, i);
            testBit = getBit<uint64_t>(totest, i);
            if (markerBit != testBit)
                errors++;

            if (errors > MAX_ERROR)
                return errors;
        }
        return errors;
    }

    SVISSRDeframer::SVISSRDeframer()
    {
        // Default values
        writeFrame = false;
        wroteBits = 0;
        outputBits = 0;
        goodBits = 0;
    }

    // Write a single bit into the frame
    void SVISSRDeframer::pushBit(uint8_t bit)
    {
        byteBuffer = (byteBuffer << 1) | bit;
        wroteBits++;
        if (wroteBits == 8)
        {
            frameBuffer.push_back(byteBuffer);
            wroteBits = 0;
        }
    }

    std::vector<std::vector<uint8_t>> SVISSRDeframer::work(uint8_t *data, int len)
    {
        // Output buffer
        std::vector<std::vector<uint8_t>> framesOut;

        // Loop in all bytes
        for (int byten = 0; byten < len; byten++)
        {
            uint8_t &byte = data[byten];

            // Loop in all bits!
            for (int i = 7; i >= 0; i--)
            {
                // Get a bit, push it
                uint8_t bit = getBit<uint8_t>(byte, i);

                shifter = shifter << 1 | bit;

                // Writing a frame!
                if (writeFrame)
                {
                    // First run : push header
                    if (outputBits == 0)
                    {
                        /*SYNC_T syncAsm = ASM_SYNC;
                    for (int y = SYNC_SIZE - 1; y >= 0; y--)
                    {
                        pushBit(getBit<SYNC_T>(syncAsm, y));
                        outputBits++;
                    }*/
                        // pushBit(0);
                        // outputBits++;
                    }

#if 0
                    // New ASM, ABORT! and process the new one
                    if (outputBits > 10000)
                        if (checkSyncMarker(0b0100101110111011101110011001100110010101010101010111111111111111, shifter) < MAX_ERROR)
                        {
                            // Fill up what we're missing
                            for (int b = 0; b < 354848 - outputBits; b++)
                                pushBit(0);

                            writeFrame = false;
                            wroteBits = 0;
                            outputBits = 0;
                            framesOut.push_back(frameBuffer);
                            frameBuffer.clear();

                            writeFrame = true;

                            continue;
                        }
#endif

                    // Push current bit
                    pushBit(bit);
                    outputBits++;

                    // Once we wrote a frame, exit!
                    if (outputBits == 354848)
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
                if (checkSyncMarker(0b0100101110111011101110011001100110010101010101010111111111111111, shifter) < MAX_ERROR)
                    writeFrame = true;
            }
        }

        // Output what we found if anything
        return framesOut;
    }
}
