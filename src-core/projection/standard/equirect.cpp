#include "equirect.h"

namespace proj
{
    namespace
    {
        struct projection_equirect_t
        {
        };
    }

    bool projection_equirect_setup(projection_t *proj)
    {
        projection_equirect_t *ptr = (projection_equirect_t *)malloc(sizeof(projection_equirect_t));
        proj->proj_dat = ptr;

        return false;
    }

    bool projection_equirect_fwd(const projection_t * /*proj*/, double lam, double phi, double *x, double *y)
    {
        // projection_equirect_t *ptr = (projection_equirect_t *)proj->proj_dat;

        *x = lam * RAD2DEG;
        *y = phi * RAD2DEG;

        return false;
    }

    bool projection_equirect_inv(const projection_t * /*proj*/, double x, double y, double *lam, double *phi)
    {
        // projection_equirect_t *ptr = (projection_equirect_t *)proj->proj_dat;

        *phi = y * DEG2RAD;
        *lam = x * DEG2RAD;

        return false;
    }
}