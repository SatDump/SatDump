#pragma once

/**
 * @file j2k_utils.h
 */

#include <cstdint>
#include "common/image/image.h"

namespace image
{
    /**
     * @brief  Decompress J2K Data from memory with OpenJP2, returns a 16-bit image
     * @param data input buffer
     * @param length buffer size
     * @return the image
     */
    Image decompress_j2k_openjp2(uint8_t *data, int length);
}