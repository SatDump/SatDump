#pragma once

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace image
{
    namespace earth_curvature
    {
        cimg_library::CImg<unsigned short> correct_earth_curvature(cimg_library::CImg<unsigned short> &image, float satellite_height, float swath, float resolution_km);
    }
}