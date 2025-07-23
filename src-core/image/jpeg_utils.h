#pragma once

/**
 * @file jpeg_utils.h
 * @brief Utilities specific to JPEG manipulation (from memory)
 */

#include "image.h"
#include <cstdint>
#include <vector>

namespace satdump
{
    namespace image
    {
        /**
         * @brief Decompress 8-bit JPEG Data from memory
         * @param data input buffer
         * @param length buffer size
         * @param ignore_errors if true, ignore some errors (needed for a
         * few satellites sending slightly malformed but still valid JPEGs)
         * @return the image
         */
        Image decompress_jpeg(uint8_t *data, int length, bool ignore_errors = false);

        /**
         * @brief Compress JPEG to memory
         * @param img the image to compress (8-bit)
         * @return vector containing the JPEG stream
         */
        std::vector<uint8_t> save_jpeg_mem(Image &img);

        // Decompress J2K Data from memory with OpenJP2
        // Image<uint16_t> decompress_j2k_openjp2(uint8_t *data, int length);
    } // namespace image
} // namespace satdump