#include "gvar_derand.h"

template <typename T>
inline bool getBit(T data, int bit)
{
    return (data >> bit) & 1;
}

namespace goes
{
    namespace gvar
    {
        PNDerandomizer::PNDerandomizer()
        {
            derandTable = new uint8_t[26150];

            // Generate derandomization table
            int oo = 0;
            uint8_t currentByteShifter = 0;
            int inCurrentByteShiter = 0;

            uint16_t shifter = 0b101001110110101;

            for (int i = 0; i < 10032 + 209200; i++)
            {
                bool xor1 = getBit<uint16_t>(shifter, 14);
                bool xor2 = getBit<uint16_t>(shifter, 7);
                bool outBit = xor1 ^ xor2;

                shifter = shifter << 1 | outBit;

                if (i >= 10032)
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
            for (int byten = 0; byten < length - 8; byten++)
            {
                if (byten % 2 != 0)
                    frame[8 + byten] = (derandTable[byten] ^ frame[8 + byten]) ^ 0xFF;
                else
                    frame[8 + byten] = derandTable[byten] ^ frame[8 + byten];
            }
        }
    }
}