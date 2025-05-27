#pragma once

/**
 * @file brightness_contrast.h
 */

#include "image.h"
#include <cstdint>

namespace satdump
{
    namespace image
    {
        /**
         * @brief Constrast and brightness correction, similar to Gimp
         *
         * @param image the image to use
         * @param brightness brightness factor
         * @param contrast contrast factor
         */
        void brightness_contrast(Image &image, float brightness, float contrast);
    } // namespace image
} // namespace satdump