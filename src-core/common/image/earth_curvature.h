#pragma once

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace image
{
    namespace earth_curvature
    {
        template <typename T>
        cimg_library::CImg<T> correct_earth_curvature(cimg_library::CImg<T> &image, float satellite_height, float swath, float resolution_km);
    }
}