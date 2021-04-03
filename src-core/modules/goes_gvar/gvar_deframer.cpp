#include "gvar_deframer.h"

#include <math.h>

// Returns the asked bit!
template <typename T>
inline bool getBit(T &data, int &bit)
{
    return (data >> bit) & 1;
}

namespace goes_gvar
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

            if (errors >= 10)
                return errors;
        }
        return errors;
    }

    template <typename SYNC_T, int SYNC_SIZE, int FRAME_SIZE, SYNC_T ASM_SYNC>
    GVARDeframer<SYNC_T, SYNC_SIZE, FRAME_SIZE, ASM_SYNC>::GVARDeframer()
    {
        // Default values
        writeFrame = false;
        wroteBits = 0;
        outputBits = 0;
    }

    // Write a single bit into the frame
    template <typename SYNC_T, int SYNC_SIZE, int FRAME_SIZE, SYNC_T ASM_SYNC>
    void GVARDeframer<SYNC_T, SYNC_SIZE, FRAME_SIZE, ASM_SYNC>::pushBit(uint8_t bit)
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
    std::vector<std::vector<uint8_t>> GVARDeframer<SYNC_T, SYNC_SIZE, FRAME_SIZE, ASM_SYNC>::work(uint8_t *data, int len)
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

                    // New ASM, ABORT! and process the new one
                    if (outputBits > 10000)
                        if (checkSyncMarker(ASM_SYNC, shifter) < 10)
                        {
                            // Fill up what we're missing
                            for (int b = 0; b < FRAME_SIZE - outputBits; b++)
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
                if (checkSyncMarker(ASM_SYNC, shifter) < 10)
                {
                    writeFrame = true;
                }
            }
        }

        // Output what we found if anything
        return framesOut;
    }

    // Build this template for GVAR data
    template class GVARDeframer<uint64_t, 64, 262288, 0b0001101111100111110100000001111110111111100000001111111111111110>;
}