#pragma once

#include <cstdint>
#include <vector>

namespace spacex
{
    // Struct representing a M-PDU
    struct SpaceXHeader
    {
        uint32_t frame_counter;
        uint16_t first_header_pointer;
        uint8_t *data;
    };

    // Parse MPDU from CADU
    SpaceXHeader parseHeader(uint8_t *cadu);
} // namespace proba