#pragma once

#include <cstdint>
#include "image.h"

namespace image2
{
    // Decompress JPEG Data from memory
    Image decompress_jpeg12(uint8_t *data, int length, bool ignore_errors = false);
}