#pragma once

/**
 * @file simple_mean_filter.h
 * @brief Simple mean-based smoothing, sharpening and edge filter
 */

#include "image.h"
#include <cstdint>

namespace satdump
{
    namespace image
    {
        enum simple_mean_filter_mode_t
        {
            SMOOTH,
            SHARPEN,
            EDGE,
        };

        /**
         * @brief Simple mean-based smoothing, sharpening and edge filter.
         *
         * @param image the image to use
         * @param radius radius of the square used to average pixels
         * @param mode smooth, sharpen or edge
         */
        void simple_mean_filter(Image &image, int radius, simple_mean_filter_mode_t mode);
    } // namespace image
} // namespace satdump