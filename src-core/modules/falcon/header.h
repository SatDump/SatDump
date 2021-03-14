#pragma once

#include <cstdint>
#include <vector>

namespace falcon
{
    // Struct representing a M-PDU
    struct FalconHeader
    {
        uint32_t frame_counter;
        uint16_t first_header_pointer;
        uint8_t *data;
    };

    // Parse MPDU from CADU
    FalconHeader parseHeader(uint8_t *cadu);
} // namespace proba