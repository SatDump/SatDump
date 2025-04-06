#pragma once

#include <cmath>
#include <string>
#ifndef M_PI
#define M_PI 3.14159265359
#endif
#define DEG_TO_RAD (M_PI / 180.0)
#define RAD_TO_DEG (180.0 / M_PI)

namespace geodetic
{
    // Simple class to hold Geodetic coordinates and easily convert between degs and rads
    class geodetic_coords_t
    {
    public:
        bool is_radians = false;

    public:
        double lat = 0; // Latitude, degrees by default
        double lon = 0; // Longitude, degrees by default
        double alt = 0; // Altitude, kilometers

        geodetic_coords_t();
        geodetic_coords_t(double lat, double lon, double alt, bool radians = false);
        geodetic_coords_t toRads(); // Convert to Rads if not done already
        geodetic_coords_t toDegs(); // Convert to Degs if not done already
        std::string str();
    };
};