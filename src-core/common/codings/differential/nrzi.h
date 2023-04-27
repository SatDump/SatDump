#pragma once

#include <cstdint>

namespace diff
{
    // Countinuous decoder
    class NRZIDiff
    {
    private:
        uint8_t lastBit = 0;

    public:
        void decode_bits(uint8_t *data, int length); // Decode on bits
    };
} // namespace diff