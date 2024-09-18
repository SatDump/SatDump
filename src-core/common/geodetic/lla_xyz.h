#pragma once

#include "geodetic_coordinates.h"

namespace geodetic
{
    struct vector
    {
        double x;
        double y;
        double z;
    };

    // Must already be in radians!
    void lla2xyz(geodetic::geodetic_coords_t lla, vector &position);
    void xyz2lla(vector position, geodetic::geodetic_coords_t &lla);
}