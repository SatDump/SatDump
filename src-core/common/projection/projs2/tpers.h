#pragma once

#include "proj.h"

namespace proj
{
    bool projection_tpers_setup(projection_t *proj, double height, double tilt, double azi);
    bool projection_tpers_fwd(projection_t *proj, double lam, double phi, double *x, double *y);
    bool projection_tpers_inv(projection_t *proj, double x, double y, double *lam, double *phi);
}