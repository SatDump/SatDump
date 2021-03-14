#pragma once

#include <cstdint>
#include <vector>

namespace falcon
{
    // Struct representing
    struct FalconPacket
    {
        int length;
        std::vector<uint8_t> payload;
    };
} // namespace proba