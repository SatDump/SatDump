#pragma once

#include "euler_coordinates.h"
#include "geodetic_coordinates.h"

namespace geodetic
{
    /*
    Given a geodetic position and orientation in Euler coordinates,
    "trace" a way down to the WGS84 ellipsoid's surface and return
    the geodetic position.
    Returns 1 on error.
    */
    int raytrace_to_earth_old(geodetic_coords_t position, euler_coords_t pointing, geodetic_coords_t &earth_point);

    int raytrace_to_earth(double time, const double position_in[3], const double velocity_in[3], geodetic::euler_coords_t pointing, geodetic::geodetic_coords_t &earth_point);
};