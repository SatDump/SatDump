#pragma once

#include <cstdint>
#include "image.h"

namespace image
{
    // Contrast and brightness correction
    void brightness_contrast(Image &image, float brightness, float contrast);
}