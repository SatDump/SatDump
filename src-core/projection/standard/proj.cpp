#include "proj.h"

#include "common/geodetic/wgs84.h"

// All projections
#include "equirect.h"
#include "stereo.h"
#include "tmerc.h"
#include "geos.h"
#include "tpers.h"
#include "webmerc.h"
// #include "lamcc.h"

namespace proj
{
    bool projection_setup(projection_t *proj)
    {
        // Set ellipsoid
        proj->a = geodetic::WGS84::a * 1e3; // To Meters!
        proj->e = geodetic::WGS84::e;
        proj->es = geodetic::WGS84::es;
        proj->n = geodetic::WGS84::n;
        proj->one_es = geodetic::WGS84::one_es;
        proj->rone_es = 1.0 / geodetic::WGS84::one_es;

        bool proj_ret = false;
        if (proj->type == ProjType_Equirectangular)
            proj_ret = projection_equirect_setup(proj);
        else if (proj->type == ProjType_Stereographic)
            proj_ret = projection_stereo_setup(proj);
        else if (proj->type == ProjType_UniversalTransverseMercator)
            proj_ret = projection_tmerc_setup(proj, proj->params.zone, proj->params.south);
        else if (proj->type == ProjType_Geos)
            proj_ret = projection_geos_setup(proj, proj->params.altitude, proj->params.sweep_x);
        else if (proj->type == ProjType_Tpers)
            proj_ret = projection_tpers_setup(proj, proj->params.altitude, proj->params.tilt * DEG2RAD, proj->params.azimuth * DEG2RAD);
        else if (proj->type == ProjType_WebMerc)
            proj_ret = projection_webmerc_setup(proj);
        else
            return true;

        if (proj_ret) // Exit on error!
            return true;

        return false;
    }

    void projection_free(projection_t *proj)
    {
        if (proj->proj_dat != nullptr)
            free(proj->proj_dat);
    }

    bool projection_perform_fwd(const projection_t *proj, double lon, double lat, double *x, double *y)
    {
        /* Convert to radians */
        lon *= DEG2RAD;
        lat *= DEG2RAD;

        lon -= proj->lam0; // Lon Shift

        /* Call projection function */
        bool proj_ret = false;
        switch (proj->type)
        {
        case ProjType_Equirectangular:
            proj_ret = projection_equirect_fwd(proj, lon, lat, x, y);
            break;

        case ProjType_Stereographic:
            proj_ret = projection_stereo_fwd(proj, lon, lat, x, y);
            break;

        case ProjType_UniversalTransverseMercator:
            proj_ret = projection_tmerc_fwd(proj, lon, lat, x, y);
            break;

        case ProjType_Geos:
            proj_ret = projection_geos_fwd(proj, lon, lat, x, y);
            break;

        case ProjType_Tpers:
            proj_ret = projection_tpers_fwd(proj, lon, lat, x, y);
            break;

        case ProjType_WebMerc:
            proj_ret = projection_webmerc_fwd(proj, lon, lat, x, y);
            break;

        default:
            break;
        }

        if (proj_ret) // Exit on error!
            return true;

        /* Apply scalars & offsets */
        if (proj->type != ProjType_Equirectangular)
        {
            *x *= proj->a;
            *y *= proj->a;
        }

        *x += proj->x0;
        *y += proj->y0;

        *x = (*x - proj->proj_offset_x) / proj->proj_scalar_x;
        *y = (*y - proj->proj_offset_y) / proj->proj_scalar_y;

        return false;
    }

    bool projection_perform_inv(const projection_t *proj, double x, double y, double *lon, double *lat)
    {
        /* Apply scalars & offsets */
        x = x * proj->proj_scalar_x + proj->proj_offset_x;
        y = y * proj->proj_scalar_y + proj->proj_offset_y;

        x -= proj->x0;
        y -= proj->y0;

        if (proj->type != ProjType_Equirectangular)
        {
            x *= (1.0 / proj->a);
            y *= (1.0 / proj->a);
        }

        /* Call projection function */
        bool proj_ret = false;
        switch (proj->type)
        {
        case ProjType_Equirectangular:
            proj_ret = projection_equirect_inv(proj, x, y, lon, lat);
            break;

        case ProjType_Stereographic:
            proj_ret = projection_stereo_inv(proj, x, y, lon, lat);
            break;

        case ProjType_UniversalTransverseMercator:
            proj_ret = projection_tmerc_inv(proj, x, y, lon, lat);
            break;

        case ProjType_Geos:
            proj_ret = projection_geos_inv(proj, x, y, lon, lat);
            break;

        case ProjType_Tpers:
            proj_ret = projection_tpers_inv(proj, x, y, lon, lat);
            break;

        case ProjType_WebMerc:
            proj_ret = projection_webmerc_inv(proj, x, y, lon, lat);
            break;

        default:
            break;
        }

        if (proj_ret) // Exit on error!
            return true;

        /* Convert to degrees */
        *lon += proj->lam0; // Lon Shift

        while (*lon < -M_PI)
            *lon += M_PI * 2;
        while (*lon > M_PI)
            *lon -= M_PI * 2;

        *lon *= RAD2DEG;
        *lat *= RAD2DEG;

        return false;
    }
}
