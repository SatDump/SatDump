#pragma once

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace image
{
    // Implementation of GIMP's White Balance algorithm
    void white_balance(cimg_library::CImg<unsigned short> &image, float percentileValue = 0.05f, int channelCount = 3);
}