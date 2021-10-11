#pragma once

#include "geodetic_coordinates.h"

namespace geodetic
{
    struct geodetic_curve_t
    {
        double distance;
        double azimuth;
        double reverse_azimuth;

        bool is_radians = false;

        geodetic_curve_t(double distance, double azimuth, double reverse_azimuth, bool radians = false)
        {
            this->distance = distance;
            this->azimuth = azimuth;
            this->reverse_azimuth = reverse_azimuth;
            this->is_radians = radians;
        }

        geodetic_curve_t toRads() // Convert to Rads if not done already
        {
            if (!is_radians)
            {
                azimuth *= DEG_TO_RAD;
                reverse_azimuth *= DEG_TO_RAD;
                is_radians = true;
            }

            return *this;
        }

        geodetic_curve_t toDegs() // Convert to Degs if not done already
        {
            if (is_radians)
            {
                azimuth *= RAD_TO_DEG;
                reverse_azimuth *= RAD_TO_DEG;
                is_radians = false;
            }

            return *this;
        }
    };

    // Compute new position and bearing given start geodetic coordinates, bearing and displacement distance
    geodetic_coords_t vincentys_forward(geodetic_coords_t start, double initialBearing, double distance, double &finalBearing, double tolerance = 1e-13);

    // Compute distance and bearing between 2 geodetic coordinates
    geodetic_curve_t vincentys_inverse(geodetic_coords_t start, geodetic_coords_t end, double tolerance = 1e-13);
};