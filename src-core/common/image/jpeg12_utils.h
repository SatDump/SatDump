#pragma once

/**
 * @file jpeg12_utils.h
 */

#include <cstdint>
#include "image.h"

namespace image
{
    /**
     * @brief Decompress 12-bit JPEG Data from memory
     * @param data input buffer
     * @param length buffer size
     * @param ignore_errors if true, ignore some errors (needed for a
     * few satellites sending slightly malformed but still valid JPEGs)
     */
    Image decompress_jpeg12(uint8_t *data, int length, bool ignore_errors = false);
}