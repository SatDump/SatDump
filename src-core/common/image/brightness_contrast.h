#pragma once

/**
 * @file brightness_contrast.h
 */

#include <cstdint>
#include "image.h"

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
}