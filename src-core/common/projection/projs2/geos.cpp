#include "geos.h"

namespace proj
{
    namespace
    {
        struct projection_geos_t
        {
            double h;
            double radius_p;
            double radius_p2;
            double radius_p_inv2;
            double radius_g;
            double radius_g_1;
            double C;
            int flip_axis;
        };
    }

    bool projection_geos_setup(projection_t *proj, double altitude, bool sweep_x)
    {
        projection_geos_t *ptr = (projection_geos_t *)malloc(sizeof(projection_geos_t));
        proj->proj_dat = ptr;

        ptr->h = altitude;
        ptr->flip_axis = sweep_x;

        ptr->radius_g_1 = ptr->h / proj->a;
        if (ptr->radius_g_1 <= 0 || ptr->radius_g_1 > 1e10)
        {
            return true;
        }
        ptr->radius_g = 1. + ptr->radius_g_1;
        ptr->C = ptr->radius_g * ptr->radius_g - 1.0;
        if (proj->es != 0.0)
        {
            ptr->radius_p = sqrt(proj->one_es);
            ptr->radius_p2 = proj->one_es;
            ptr->radius_p_inv2 = proj->rone_es;
        }
        else
        {
            ptr->radius_p = ptr->radius_p2 = ptr->radius_p_inv2 = 1.0;
            return true;
        }

        return false;
    }

    bool projection_geos_fwd(projection_t *proj, double lam, double phi, double *x, double *y)
    {
        projection_geos_t *ptr = (projection_geos_t *)proj->proj_dat;

        double r, Vx, Vy, Vz, tmp;

        /* Calculation of geocentric latitude. */
        phi = atan(ptr->radius_p2 * tan(phi));

        /* Calculation of the three components of the vector from satellite to
        ** position on earth surface (long,lat).*/
        r = (ptr->radius_p) / hypot(ptr->radius_p * cos(phi), sin(phi));
        Vx = r * cos(lam) * cos(phi);
        Vy = r * sin(lam) * cos(phi);
        Vz = r * sin(phi);

        /* Check visibility. */
        if (((ptr->radius_g - Vx) * Vx - Vy * Vy - Vz * Vz * ptr->radius_p_inv2) < 0.)
        {
            return true;
        }

        /* Calculation based on view angles from satellite. */
        tmp = ptr->radius_g - Vx;

        if (ptr->flip_axis)
        {
            *x = ptr->radius_g_1 * atan(Vy / hypot(Vz, tmp));
            *y = ptr->radius_g_1 * atan(Vz / tmp);
        }
        else
        {
            *x = ptr->radius_g_1 * atan(Vy / tmp);
            *y = ptr->radius_g_1 * atan(Vz / hypot(Vy, tmp));
        }

        return false;
    }

    bool projection_geos_inv(projection_t *proj, double x, double y, double *lam, double *phi)
    {
        projection_geos_t *ptr = (projection_geos_t *)proj->proj_dat;

        double Vx, Vy, Vz, a, b, k;

        /* Setting three components of vector from satellite to position.*/
        Vx = -1.0;

        if (ptr->flip_axis)
        {
            Vz = tan(y / ptr->radius_g_1);
            Vy = tan(x / ptr->radius_g_1) * hypot(1.0, Vz);
        }
        else
        {
            Vy = tan(x / ptr->radius_g_1);
            Vz = tan(y / ptr->radius_g_1) * hypot(1.0, Vy);
        }

        /* Calculation of terms in cubic equation and determinant.*/
        a = Vz / ptr->radius_p;
        a = Vy * Vy + a * a + Vx * Vx;
        b = 2 * ptr->radius_g * Vx;
        const double det = (b * b) - 4 * a * ptr->C;
        if (det < 0.)
        {
            return true;
        }

        /* Calculation of three components of vector from satellite to position.*/
        k = (-b - sqrt(det)) / (2. * a);
        Vx = ptr->radius_g + k * Vx;
        Vy *= k;
        Vz *= k;

        /* Calculation of longitude and latitude.*/
        *lam = atan2(Vy, Vx);
        *phi = atan(Vz * cos(*lam) / Vx);
        *phi = atan(ptr->radius_p_inv2 * tan(*phi));

        return false;
    }
}