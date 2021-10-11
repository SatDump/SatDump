#include "geodetic_coordinates.h"

namespace geodetic
{
    geodetic_coords_t::geodetic_coords_t()
    {
    }

    geodetic_coords_t::geodetic_coords_t(double lat, double lon, double alt, bool radians)
    {
        this->lat = lat;
        this->lon = lon;
        this->alt = alt;
        this->is_radians = radians;
    }

    geodetic_coords_t geodetic_coords_t::toRads() // Convert to Rads if not done already
    {
        if (!is_radians)
        {
            lat *= DEG_TO_RAD;
            lon *= DEG_TO_RAD;
            is_radians = true;
        }

        return *this;
    }

    geodetic_coords_t geodetic_coords_t::toDegs() // Convert to Degs if not done already
    {
        if (is_radians)
        {
            lat *= RAD_TO_DEG;
            lon *= RAD_TO_DEG;
            is_radians = false;
        }

        return *this;
    }

    std::string geodetic_coords_t::str()
    {
        if (is_radians)
            return "Lat: " + std::to_string(lat * RAD_TO_DEG) + ", Lon: " + std::to_string(lon * RAD_TO_DEG) + ", Alt: " + std::to_string(alt);
        else
            return "Lat: " + std::to_string(lat) + ", Lon: " + std::to_string(lon) + ", Alt: " + std::to_string(alt);
    }
};
