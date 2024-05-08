#pragma once

#include "image.h"

namespace image2
{
    namespace earth_curvature
    {
        Image correct_earth_curvature(Image &image, float satellite_height, float swath, float resolution_km, float *foward_table = nullptr);
    }
}