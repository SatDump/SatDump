#pragma once

#include <cstdint>
#include "image.h"

namespace image
{
    // Decompress JPEG Data from memory
    Image<uint8_t> decompress_jpeg(uint8_t *data, int length, bool ignore_errors = false);

    // Decompress J2K Data from memory with OpenJP2
    Image<uint16_t> decompress_j2k_openjp2(uint8_t *data, int length);
}