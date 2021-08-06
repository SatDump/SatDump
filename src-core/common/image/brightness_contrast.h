#pragma once

#include <cstdint>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace image
{
    // Contrast and brightness correction
    template <typename T>
    void brightness_contrast(cimg_library::CImg<T> &image, float brightness, float contrast, int channelCount = 3);
}