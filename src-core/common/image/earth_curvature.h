#pragma once

#include "image.h"

namespace image
{
    namespace earth_curvature
    {
        template <typename T>
        Image<T> correct_earth_curvature(Image<T> &image, float satellite_height, float swath, float resolution_km);
    }
}