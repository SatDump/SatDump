#pragma once

#include <cmath>
#include <string>
#ifndef M_PI
#define M_PI (3.14159265359)
#endif
#define DEG_TO_RAD (M_PI / 180.0)
#define RAD_TO_DEG (180.0 / M_PI)

namespace geodetic
{
    // Simple class to hold Geodetic coordinates and easily convert between degs and rads
    class euler_coords_t
    {
    private:
        bool is_radians = false;

    public:
        double roll = 0;  // Roll, degrees by default
        double pitch = 0; // Pitch, degrees by default
        double yaw = 0;   // Yaw, degrees by default

        euler_coords_t();
        euler_coords_t(double roll, double pitch, double yaw, bool radians = false);
        euler_coords_t toRads(); // Convert to Rads if not done already
        euler_coords_t toDegs(); // Convert to Degs if not done already
        std::string str();
    };
};