#pragma once

#include "projection/standard/proj.h"
#include "projection/standard/proj_json.h"
#include <cmath>

namespace satdump_test_proj_standard
{
    // Owns a proj::projection_t and releases its private data on scope exit,
    // including when projection_setup() failed after allocating.
    struct ScopedProj
    {
        proj::projection_t p;

        ScopedProj() {}
        explicit ScopedProj(const nlohmann::json &cfg) { p = cfg.get<proj::projection_t>(); }
        ScopedProj(const ScopedProj &) = delete;
        ScopedProj &operator=(const ScopedProj &) = delete;
        ~ScopedProj() { proj::projection_free(&p); }

        // Returns true on success (inverse of the C-style error convention)
        bool setup() { return !proj::projection_setup(&p); }
    };

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
} // namespace satdump_test_proj_standard
