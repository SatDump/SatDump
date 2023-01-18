#pragma once

#include <vector>
#include <cstdint>

namespace png_fix
{
    bool repair_png(std::vector<uint8_t> in, std::vector<uint8_t> &out);
}