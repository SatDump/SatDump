#pragma once

/**
 * @file j2k_utils.h
 * @brief Utilities specific to JPEG2000 manipulation (from memory)
 */

#include "image/image.h"
#include <cstdint>

namespace satdump
{
    namespace image
    {
        /**
         * @brief  Decompress J2K Data from memory with OpenJP2, returns a 16-bit image
         * @param data input buffer
         * @param length buffer size
         * @return the image
         */
        Image decompress_j2k_openjp2(uint8_t *data, int length);
    } // namespace image
} // namespace satdump