#pragma once

#include "proj.h"

namespace proj
{
    bool projection_tmerc_setup(projection_t *proj, int zone = -1, bool south = false);
    bool projection_tmerc_fwd(const projection_t *proj, double lam, double phi, double *x, double *y);
    bool projection_tmerc_inv(const projection_t *proj, double x, double y, double *lam, double *phi);
}