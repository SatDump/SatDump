#pragma once

#include "projection/proj_test_helpers.h"
#include "projection/standard/proj.h"
#include "projection/standard/proj_json.h"

namespace proj_test
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
} // namespace proj_test
