#pragma once

#include <cstdint>
#include <vector>

namespace spacex
{
    // Struct representing
    struct SpaceXPacket
    {
        int length;
        std::vector<uint8_t> payload;
    };
} // namespace proba