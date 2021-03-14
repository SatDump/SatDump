#include "header.h"
#include <cstdint>
#include <cmath>

namespace falcon
{
    // Parse MPDU from CADU
    FalconHeader parseHeader(uint8_t *cadu)
    {
        uint32_t frame_counter = cadu[4] << 12 | cadu[5] << 4 | cadu[6] >> 3;
        uint16_t first_header_pointer = (cadu[6] & 0b00000111) << 8 | cadu[7];

        return {frame_counter, first_header_pointer, &cadu[8]};
    }
} // namespace proba