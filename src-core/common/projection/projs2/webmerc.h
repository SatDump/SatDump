#pragma once

#include "proj.h"

namespace proj
{
    bool projection_webmerc_setup(projection_t *proj);
    bool projection_webmerc_fwd(const projection_t *proj, double lam, double phi, double *x, double *y);
    bool projection_webmerc_inv(const projection_t *proj, double x, double y, double *lam, double *phi);
}