#pragma once

/**
 * @file jpeg12_utils.h
 * @brief Utilities specific to 12-bit JPEG manipulation (from memory)
 */

#include "image.h"
#include <cstdint>

namespace satdump
{
    namespace image
    {
        /**
         * @brief Decompress 12-bit JPEG Data from memory
         * @param data input buffer
         * @param length buffer size
         * @param ignore_errors if true, ignore some errors (needed for a
         * few satellites sending slightly malformed but still valid JPEGs)
         * @return the image
         */
        Image decompress_jpeg12(uint8_t *data, int length, bool ignore_errors = false);
    } // namespace image
} // namespace satdump