#pragma once

#include "image.h"

namespace image
{
    namespace earth_curvature
    {
        Image correct_earth_curvature(Image &image, float satellite_height, float swath, float resolution_km, std::vector<float> *foward_table = nullptr, std::vector<float> *reverse_table = nullptr);
        image::Image perform_geometric_correction(image::Image img, bool &success, std::vector<float> *foward_table = nullptr, std::vector<float> *reverse_table = nullptr); // TODOREWORK VECTORS FOR BOTH
    }
}