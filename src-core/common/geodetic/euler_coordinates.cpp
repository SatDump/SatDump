#include "euler_coordinates.h"

namespace geodetic
{
    euler_coords_t::euler_coords_t()
    {
    }

    euler_coords_t::euler_coords_t(double roll, double pitch, double yaw, bool radians)
    {
        this->roll = roll;
        this->pitch = pitch;
        this->yaw = yaw;
        this->is_radians = radians;
    }

    euler_coords_t euler_coords_t::toRads() // Convert to Rads if not done already
    {
        if (!is_radians)
        {
            roll *= DEG_TO_RAD;
            pitch *= DEG_TO_RAD;
            yaw *= DEG_TO_RAD;
            is_radians = true;
        }

        return *this;
    }

    euler_coords_t euler_coords_t::toDegs() // Convert to Degs if not done already
    {
        if (is_radians)
        {
            roll *= RAD_TO_DEG;
            pitch *= RAD_TO_DEG;
            yaw *= RAD_TO_DEG;
            is_radians = false;
        }

        return *this;
    }

    std::string euler_coords_t::str()
    {
        if (is_radians)
            return "Roll: " + std::to_string(roll * RAD_TO_DEG) + ", Pitch: " + std::to_string(pitch * RAD_TO_DEG) + ", Yaw: " + std::to_string(yaw * RAD_TO_DEG);
        else
            return "Roll: " + std::to_string(roll) + ", Pitch: " + std::to_string(pitch) + ", Yaw: " + std::to_string(yaw);
    }
};
