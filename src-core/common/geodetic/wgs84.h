#pragma once

namespace geodetic
{
    /*
    WGS86 ellipsoid definition, used everywhere in SatDump (or should be!)
    */
    namespace WGS84
    {
        const double a = 6378.137;                                   // Semimajor Axis
        const double rf = 298.257223563;                             // Inverse flattening
        const double f = 1.0 / rf;                                   // Flattening
        const double b = a * (1 - f);                                // Semiminor Axis
        const double e = sqrt((pow(a, 2) - pow(b, 2)) / pow(a, 2));  // First eccentricity
        const double e2 = sqrt((pow(a, 2) - pow(b, 2)) / pow(b, 2)); // Second eccentricity
        const double es = pow(e, 2);                                 // First eccentricity^2
        const double one_es = 1.0 - es;                              // First 1 - eccentricity^2
        const double alpha = asin(e);                                // Angular eccentricity
        const double n = pow(tan(alpha / 2), 2);                     // Third flattening
    };
};