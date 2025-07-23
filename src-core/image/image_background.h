#pragma once

/**
 * @file image_background.h
 * @brief Projection-based image cropping
 */

#include "image.h"
#include "nlohmann/json.hpp"

namespace satdump
{
    namespace image
    {
        /**
         * @brief Remove background (or non-space-views)
         * of satellite images. Usually used for GOES, but could
         * also be used to remove space off tilted M2, if it was
         * still alive...
         * @param img the image to process, with a proj_cfg metadata
         * @param progress pointer returning progress from 0 to 1
         */
        void remove_background(Image &img, float *progress);
    } // namespace image
} // namespace satdump