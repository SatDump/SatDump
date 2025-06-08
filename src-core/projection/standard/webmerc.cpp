#include "webmerc.h"

namespace proj
{
    namespace
    {
        struct projection_webmerc_t
        {
        };
    }

    bool projection_webmerc_setup(projection_t *proj)
    {
        projection_webmerc_t *ptr = (projection_webmerc_t *)malloc(sizeof(projection_webmerc_t));
        proj->proj_dat = ptr;

        return false;
    }

    bool projection_webmerc_fwd(const projection_t *proj, double lam, double phi, double *x, double *y)
    {
        // projection_webmerc_t *ptr = (projection_webmerc_t *)proj->proj_dat;

        *x = proj->k0 * lam;
        *y = proj->k0 * asinh(tan(phi));

        return false;
    }

    bool projection_webmerc_inv(const projection_t *proj, double x, double y, double *lam, double *phi)
    {
        // projection_webmerc_t *ptr = (projection_webmerc_t *)proj->proj_dat;

        *phi = atan(sinh(y / proj->k0));
        *lam = x / proj->k0;

        return true;
    }
}
