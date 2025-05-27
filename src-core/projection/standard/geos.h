#pragma once

#include "proj.h"

namespace proj
{
    bool projection_geos_setup(projection_t *proj, double altitude = 0, bool sweep_x = false);
    bool projection_geos_fwd(const projection_t *proj, double lam, double phi, double *x, double *y);
    bool projection_geos_inv(const projection_t *proj, double x, double y, double *lam, double *phi);
}