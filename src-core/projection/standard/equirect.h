#pragma once

#include "proj.h"

namespace proj
{
    bool projection_equirect_setup(projection_t *proj);
    bool projection_equirect_fwd(const projection_t *proj, double lam, double phi, double *x, double *y);
    bool projection_equirect_inv(const projection_t *proj, double x, double y, double *lam, double *phi);
}