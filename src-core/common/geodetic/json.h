#pragma once

#include "nlohmann/json.hpp"
#include "geodetic_coordinates.h"

namespace geodetic
{
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(geodetic_coords_t, lat, lon, alt, is_radians)
}