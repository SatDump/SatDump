#pragma once

/**
 * @file brightness_contrast.h
 * @brief Brightness/Contrast manipulation
 */

#include "image.h"
#include <cstdint>

namespace satdump
{
    namespace image
    {
        /**
         * @brief contrast and brightness correction, similar to Gimp
         *
         * @param image the image to use
         * @param brightness brightness factor
         * @param contrast contrast factor
         */
        void brightness_contrast(Image &image, float brightness, float contrast);
    } // namespace image
} // namespace satdump