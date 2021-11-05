#pragma once

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace fengyun3
{
    namespace mersi
    {
        cimg_library::CImg<unsigned short> &banding_correct(cimg_library::CImg<unsigned short> &imageo, int rowSize, float percent = 0.10);
    };
};