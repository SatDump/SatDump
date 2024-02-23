#pragma once

#include "proj.h"

namespace proj
{
    bool projection_stereo_setup(projection_t *proj);
    bool projection_stereo_fwd(projection_t *proj, double lam, double phi, double *x, double *y);
    bool projection_stereo_inv(projection_t *proj, double x, double y, double *lam, double *phi);
}