#pragma once

#include "geodetic_coordinates.h"

namespace geodetic
{
    struct az_el_coords_t
    {
        double az;
        double el;
        double range; // Meters
    };

    az_el_coords_t calc_azel(geodetic_coords_t ground_pos, geodetic_coords_t obj);
}; // namespace geodetic