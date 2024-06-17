#pragma once

#include <cstdint>
#include "common/image/image.h"

namespace image
{
    // Decompress J2K Data from memory with OpenJP2
    Image decompress_j2k_openjp2(uint8_t *data, int length);
}