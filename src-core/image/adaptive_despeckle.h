#pragma once

/**
 * @file adaptive_despeckle.h
 * @brief Adaptive Despeckle from Gimp
 */

#include "image.h"
#include <cstdint>

namespace satdump
{
    namespace image
    {
        struct AdaptiveDespeckleConfig
        {
            enum filter_type_t
            {
                ADAPTIVE,
                RECURSIVE,
            };

            filter_type_t filter_type = ADAPTIVE;
            int radius = 3;
            int black_level = 0;
            int white_level = 255;
        };

        /**
         * @brief adaptive despeckle, similar to Gimp
         *
         * @param image the image to use
         * @param cfg despeckle configuration
         * @param progess optional progress variable
         */
        void adaptive_despeckle(image::Image &image, AdaptiveDespeckleConfig cfg, float *progress = nullptr);
    } // namespace image
} // namespace satdump