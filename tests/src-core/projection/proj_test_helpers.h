#pragma once

// Shared helpers for the src-core/projection tests. Everything lives in the
// proj_test namespace and is inline, as all test files link into one binary.
// projection/standard/proj_test_helpers.h adds the proj_t helpers.

#include <cmath>

namespace proj_test
{
    // Difference between two longitudes in degrees, folded into [-180, 180]
    inline double lonDiff(double a, double b)
    {
        double d = std::fmod(a - b, 360.0);
        if (d > 180)
            d -= 360;
        if (d < -180)
            d += 360;
        return d;
    }
} // namespace proj_test
