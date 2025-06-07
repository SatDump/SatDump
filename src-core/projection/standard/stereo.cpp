#include "stereo.h"

namespace proj
{
    namespace
    {
        enum Mode
        {
            S_POLE = 0,
            N_POLE = 1,
            OBLIQ = 2,
            EQUIT = 3
        };

        struct projection_stereo_t
        {
            double phits;
            double sinX1;
            double cosX1;
            double akm1;
            enum Mode mode;
        };

        double pj_tsfn(double phi, double sinphi, double e)
        {
            double cosphi = cos(phi);
            return exp(e * atanh(e * sinphi)) * (sinphi > 0 ? cosphi / (1 + sinphi) : (1 - sinphi) / cosphi);
        }

        double ssfn_(double phit, double sinphi, double eccen)
        {
            sinphi *= eccen;
            return (tan(.5 * (M_PI_2 + phit)) * pow((1. - sinphi) / (1. + sinphi), .5 * eccen));
        }
    }

#define EPS10 1.e-10
#define NITER 8
#define CONV 1.e-10

    bool projection_stereo_setup(projection_t *proj)
    {
        projection_stereo_t *ptr = (projection_stereo_t *)malloc(sizeof(projection_stereo_t));
        proj->proj_dat = ptr;

        ptr->phits = M_HALFPI;

        //  printf("k0 %f, x0 %f, y0 %f, e %f, es %f\n", proj->k0, proj->x0, proj->y0, proj->e,
        //         proj->es);

        double t;
        if (fabs((t = fabs(proj->phi0)) - M_HALFPI) < EPS10)
            ptr->mode = proj->phi0 < 0. ? S_POLE : N_POLE;
        else
            ptr->mode = t > EPS10 ? OBLIQ : EQUIT;
        ptr->phits = fabs(ptr->phits);

        if (proj->es != 0.0)
        {
            double X;

            switch (ptr->mode)
            {
            case N_POLE:
            case S_POLE:
                if (fabs(ptr->phits - M_HALFPI) < EPS10)
                    ptr->akm1 =
                        2. * proj->k0 /
                        sqrt(pow(1 + proj->e, 1 + proj->e) * pow(1 - proj->e, 1 - proj->e));
                else
                {
                    t = sin(ptr->phits);
                    ptr->akm1 = cos(ptr->phits) / pj_tsfn(ptr->phits, t, proj->e);
                    t *= proj->e;
                    ptr->akm1 /= sqrt(1. - t * t);
                }
                break;
            case EQUIT:
            case OBLIQ:
                t = sin(proj->phi0);
                X = 2. * atan(ssfn_(proj->phi0, t, proj->e)) - M_HALFPI;
                t *= proj->e;
                ptr->akm1 = 2. * proj->k0 * cos(proj->phi0) / sqrt(1. - t * t);
                ptr->sinX1 = sin(X);
                ptr->cosX1 = cos(X);
                break;
            }
        }
        else
        {
            // Spherical NOT implemented!
            return true;
        }

        return false;
    }

    bool projection_stereo_fwd(const projection_t *proj, double lam, double phi, double *x, double *y)
    {
        projection_stereo_t *ptr = (projection_stereo_t *)proj->proj_dat;

        double coslam, sinlam, sinX = 0.0, cosX = 0.0, A = 0.0, sinphi;

        coslam = cos(lam);
        sinlam = sin(lam);
        sinphi = sin(phi);
        if (ptr->mode == OBLIQ || ptr->mode == EQUIT)
        {
            const double X = 2. * atan(ssfn_(phi, sinphi, proj->e)) - M_HALFPI;
            sinX = sin(X);
            cosX = cos(X);
        }

        switch (ptr->mode)
        {
        case OBLIQ:
        {
            const double denom =
                ptr->cosX1 * (1. + ptr->sinX1 * sinX + ptr->cosX1 * cosX * coslam);
            if (denom == 0)
            {
                return true;
            }
            A = ptr->akm1 / denom;
            *y = A * (ptr->cosX1 * sinX - ptr->sinX1 * cosX * coslam);
            *x = A * cosX;
            break;
        }

        case EQUIT:
            /* avoid zero division */
            if (1. + cosX * coslam == 0.0)
            {
                *y = HUGE_VAL;
            }
            else
            {
                A = ptr->akm1 / (1. + cosX * coslam);
                *y = A * sinX;
            }
            *x = A * cosX;
            break;

        case S_POLE:
            phi = -phi;
            coslam = -coslam;
            sinphi = -sinphi;
            [[fallthrough]];
        case N_POLE:
            if (fabs(phi - M_HALFPI) < 1e-15)
                *x = 0;
            else
                *x = ptr->akm1 * pj_tsfn(phi, sinphi, proj->e);
            *y = -*x * coslam;
            break;
        }

        *x = *x * sinlam;

        return false;
    }

    bool projection_stereo_inv(const projection_t *proj, double x, double y, double *lam, double *phi)
    {
        projection_stereo_t *ptr = (projection_stereo_t *)proj->proj_dat;

        double cosphi, sinphi, tp = 0.0, phi_l = 0.0, rho, halfe = 0.0, halfpi = 0.0;
        rho = hypot(x, y);

        switch (ptr->mode)
        {
        case OBLIQ:
        case EQUIT:
            tp = 2. * atan2(rho * ptr->cosX1, ptr->akm1);
            cosphi = cos(tp);
            sinphi = sin(tp);
            if (rho == 0.0)
                phi_l = asin(cosphi * ptr->sinX1);
            else
                phi_l = asin(cosphi * ptr->sinX1 + (y * sinphi * ptr->cosX1 / rho));

            tp = tan(.5 * (M_HALFPI + phi_l));
            x *= sinphi;
            y = rho * ptr->cosX1 * cosphi - y * ptr->sinX1 * sinphi;
            halfpi = M_HALFPI;
            halfe = .5 * proj->e;
            break;
        case N_POLE:
            y = -y;
            [[fallthrough]];
        case S_POLE:
            tp = -rho / ptr->akm1;
            phi_l = M_HALFPI - 2. * atan(tp);
            halfpi = -M_HALFPI;
            halfe = -.5 * proj->e;
            break;
        }

        for (int i = NITER; i > 0; --i)
        {
            sinphi = proj->e * sin(phi_l);
            *phi = 2. * atan(tp * pow((1. + sinphi) / (1. - sinphi), halfe)) - halfpi;
            if (fabs(phi_l - *phi) < CONV)
            {
                if (ptr->mode == S_POLE)
                    *phi = -*phi;
                *lam = (x == 0. && y == 0.) ? 0. : atan2(x, y);
                return false;
            }
            phi_l = *phi;
        }

        return true;
    }
}