#include "svissr_derand.h"

template <typename T>
inline bool getBit(T data, int bit)
{
    return (data >> bit) & 1;
}

namespace fengyun_svissr
{
    PNDerandomizer::PNDerandomizer()
    {
        derandTable = new uint8_t[354848];

        // Generate derandomization table
        int oo = 0;
        uint8_t currentByteShifter = 0;
        int inCurrentByteShiter = 0;

        uint16_t shifter = 0b011001110011111;

        for (int i = 0; i < 10000 + 354848; i++)
        {
            bool xor1 = getBit<uint16_t>(shifter, 14);
            bool xor2 = getBit<uint16_t>(shifter, 13);
            bool outBit = xor1 ^ xor2;

            shifter = shifter << 1 | outBit;

            if (i >= 10000)
            {
                currentByteShifter = currentByteShifter << 1 | outBit;
                inCurrentByteShiter++;

                if (inCurrentByteShiter == 8)
                {
                    derandTable[oo++] = currentByteShifter;
                    inCurrentByteShiter = 0;
                }
            }
        }
    }

    PNDerandomizer::~PNDerandomizer()
    {
        delete[] derandTable;
    }

    void PNDerandomizer::derandData(uint8_t *frame, int length)
    {
        // Derandomize this frame
        for (int byten = 0; byten < length; byten++)
        {
            if (byten % 2 == 1)
                frame[byten] = 0xFF - (derandTable[byten] ^ frame[byten]);
            else
                frame[byten] = (derandTable[byten] ^ frame[byten]);
        }
    }
}