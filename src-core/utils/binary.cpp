#include "binary.h"

namespace satdump
{
    uint8_t reverseBits(uint8_t byte)
    {
        byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
        byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
        byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
        return byte;
    }

    uint16_t reverse16Bits(uint16_t v)
    {
        uint16_t r = 0;
        for (int i = 0; i < 16; ++i, v >>= 1)
            r = (r << 1) | (v & 0x01);
        return r;
    }
} // namespace satdump