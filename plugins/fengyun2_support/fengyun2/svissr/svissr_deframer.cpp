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

                // Get a PN bit, push it
                uint8_t pn_bit = ((pn_shifter >> 14) & 1) ^ ((pn_shifter >> 13) & 1);
                pn_shifter = (pn_shifter << 1) | pn_bit;

                // Continuously check pn_bit vs bit
                if (bit == pn_bit)
                {
                    if (pn_right_bit_counter < 5000)
                        pn_right_bit_counter++;

                    // if (pn_right_bit_counter == 1000)
                    //     printf("LOCK\n");
                }
                else
                {
                    pn_right_bit_counter -= 2;
                    if (pn_right_bit_counter <= 0)
                    {
                        pn_right_bit_counter = 1;
                        pn_shifter = shifter;
                    }
                }

                // Writing a frame!
                if (writeFrame)
                {
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
                if (checkSyncMarker(0b0100101110111011101110011001100110010101010101010111111111111111, pn_shifter) == 0 && pn_right_bit_counter > 1000)
                {
                    writeFrame = true;
                    pn_right_bit_counter = 0;
                }
            }
        }

        // Output what we found if anything
        return framesOut;
    }
} // namespace fengyun_svissr
