#pragma once

#include <cstdint>
#include "common/image/image.h"

namespace image
{
    // Decompress JPEG Data from memory
    Image<uint16_t> decompress_jpeg12(uint8_t *data, int length, bool ignore_errors = false);
}