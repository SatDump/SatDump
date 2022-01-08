#pragma once

#include <cstdint>
#include "image.h"

namespace image
{
    // Contrast and brightness correction
    template <typename T>
    void brightness_contrast(Image<T> &image, float brightness, float contrast, int channelCount = 3);
}