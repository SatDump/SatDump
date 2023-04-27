#include "nrzi.h"

namespace diff
{
    void NRZIDiff::decode_bits(uint8_t *data, int length)
    {
        uint8_t currentBit = 0;
        for (int i = 0; i < length; i++)
        {
            currentBit = data[i];
            data[i] = ~(currentBit ^ lastBit) & 1;
            lastBit = currentBit;
        }
    }
} // namespace diff