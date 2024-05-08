#pragma once

#include <cstdint>
#include "image.h"
#include <vector>

namespace image
{
    // Decompress JPEG Data from memory
    Image decompress_jpeg(uint8_t *data, int length, bool ignore_errors = false);

    // Compress JPEG to memory
    std::vector<uint8_t> save_jpeg_mem(Image &img);

    // Decompress J2K Data from memory with OpenJP2
    // Image<uint16_t> decompress_j2k_openjp2(uint8_t *data, int length);
}