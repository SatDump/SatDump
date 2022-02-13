#ifdef BUILD_TRACKING

#include "utils.h"
#include "common/geodetic/vincentys_calculations.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif
#ifndef M_2_PI
#define M_2_PI (3.14159265358979323846 * 2) /* 2 * pi */
#endif

#include <cmath>

double arccos(double x, double y)
{
    if (x && y)
    {
        if (y > 0.0)
            return acos(x / y);
        else if (y < 0.0)
            return M_PI + acos(x / y);
    }

    return 0.0;
}

/* Mirror the footprint longitude. */
bool mirror_lon(geodetic::geodetic_coords_t sat, double rangelon, double &mlon, double mapbreak)
{
    double diff;
    bool warped = false;

    /* make it so rangelon is on left of ssplon */
    diff = (sat.lon - rangelon);
    while (diff < 0)
        diff += 360;
    while (diff > 360)
        diff -= 360;

    mlon = sat.lon + fabs(diff);
    while (mlon > 180)
        mlon -= 360;
    while (mlon < -180)
        mlon += 360;
    //printf("Something %s %f %f %f\n",sat->nickname, sat->ssplon, rangelon,mapbreak);
    if (((sat.lon >= mapbreak) && (sat.lon < mapbreak + 180)) ||
        ((sat.lon < mapbreak - 180) && (sat.lon >= mapbreak - 360)))
    {
        if (((rangelon >= mapbreak) && (rangelon < mapbreak + 180)) ||
            ((rangelon < mapbreak - 180) && (rangelon >= mapbreak - 360)))
        {
        }
        else
        {
            warped = true;
            //printf ("sat %s warped for first \n",sat->nickname);
        }
    }
    else
    {
        if (((mlon >= mapbreak) && (mlon < mapbreak + 180)) ||
            ((mlon < mapbreak - 180) && (mlon >= mapbreak - 360)))
        {
            warped = true;
            //printf ("sat %s warped for second \n",sat->nickname);
        }
    }

    return warped;
}

bool north_pole_is_covered(geodetic::geodetic_coords_t sat, float footprint)
{
    int ret1;
    double qrb1, az1;

    geodetic::geodetic_curve_t curve = geodetic::vincentys_inverse(geodetic::geodetic_coords_t(0, 90, 0), sat);

    if (curve.distance <= 0.5 * footprint)
    {
        return true;
    }
    return false;
}

#endif