#include "tpers.h"

namespace proj
{
    namespace
    {
        enum Mode
        {
            N_POLE = 0,
            S_POLE = 1,
            EQUIT = 2,
            OBLIQ = 3
        };

        struct projection_tpers_t
        {
            double height;
            double sinph0;
            double cosph0;
            double p;
            double rp;
            double pn1;
            double pfact;
            double h;
            double cg;
            double sg;
            double sw;
            double cw;
            enum Mode mode;
            int tilt;
        };
    }

#define EPS10 1.e-10

    bool projection_tpers_setup(projection_t *proj, double height, double tilt, double azi)
    {
        projection_tpers_t *ptr = (projection_tpers_t *)malloc(sizeof(projection_tpers_t));
        proj->proj_dat = ptr;

        double omega = tilt;
        double gamma = azi;
        ptr->tilt = 1;
        ptr->cg = cos(gamma);
        ptr->sg = sin(gamma);
        ptr->cw = cos(omega);
        ptr->sw = sin(omega);

        ptr->height = height;

        if (fabs(fabs(proj->phi0) - M_HALFPI) < EPS10)
            ptr->mode = proj->phi0 < 0. ? S_POLE : N_POLE;
        else if (fabs(proj->phi0) < EPS10)
            ptr->mode = EQUIT;
        else
        {
            ptr->mode = OBLIQ;
            ptr->sinph0 = sin(proj->phi0);
            ptr->cosph0 = cos(proj->phi0);
        }
        ptr->pn1 = ptr->height / proj->a; /* normalize by radius */
        if (ptr->pn1 <= 0 || ptr->pn1 > 1e10)
        {
            return true;
        }
        ptr->p = 1. + ptr->pn1;
        ptr->rp = 1. / ptr->p;
        ptr->h = 1. / ptr->pn1;
        ptr->pfact = (ptr->p + 1.) * ptr->h;
        proj->es = 0.;

        return false;
    }

    bool projection_tpers_fwd(const projection_t *proj, double lam, double phi, double *x, double *y)
    {
        projection_tpers_t *ptr = (projection_tpers_t *)proj->proj_dat;

        double coslam, cosphi, sinphi;

        sinphi = sin(phi);
        cosphi = cos(phi);
        coslam = cos(lam);
        switch (ptr->mode)
        {
        case OBLIQ:
            *y = ptr->sinph0 * sinphi + ptr->cosph0 * cosphi * coslam;
            break;
        case EQUIT:
            *y = cosphi * coslam;
            break;
        case S_POLE:
            *y = -sinphi;
            break;
        case N_POLE:
            *y = sinphi;
            break;
        }
        if (*y < ptr->rp)
        {
            return true;
        }
        *y = ptr->pn1 / (ptr->p - *y);
        *x = *y * cosphi * sin(lam);
        switch (ptr->mode)
        {
        case OBLIQ:
            *y *= (ptr->cosph0 * sinphi - ptr->sinph0 * cosphi * coslam);
            break;
        case EQUIT:
            *y *= sinphi;
            break;
        case N_POLE:
            coslam = -coslam;
            [[fallthrough]];
        case S_POLE:
            *y *= cosphi * coslam;
            break;
        }
        if (ptr->tilt)
        {
            double yt, ba;

            yt = *y * ptr->cg + *x * ptr->sg;
            ba = 1. / (yt * ptr->sw * ptr->h + ptr->cw);
            *x = (*x * ptr->cg - *y * ptr->sg) * ptr->cw * ba;
            *y = yt * ba;
        }

        return false;
    }

    bool projection_tpers_inv(const projection_t *proj, double x, double y, double *lam, double *phi)
    {
        projection_tpers_t *ptr = (projection_tpers_t *)proj->proj_dat;

        double rh;

        if (ptr->tilt)
        {
            double bm, bq, yt;

            yt = 1. / (ptr->pn1 - y * ptr->sw);
            bm = ptr->pn1 * x * yt;
            bq = ptr->pn1 * y * ptr->cw * yt;
            x = bm * ptr->cg + bq * ptr->sg;
            y = bq * ptr->cg - bm * ptr->sg;
        }
        rh = hypot(x, y);
        if (fabs(rh) <= EPS10)
        {
            *lam = 0.;
            *phi = proj->phi0;
        }
        else
        {
            double cosz, sinz;
            sinz = 1. - rh * rh * ptr->pfact;
            if (sinz < 0.)
            {
                return true;
            }
            sinz = (ptr->p - sqrt(sinz)) / (ptr->pn1 / rh + rh / ptr->pn1);
            cosz = sqrt(1. - sinz * sinz);
            switch (ptr->mode)
            {
            case OBLIQ:
                *phi = asin(cosz * ptr->sinph0 + y * sinz * ptr->cosph0 / rh);
                y = (cosz - ptr->sinph0 * sin(*phi)) * rh;
                x *= sinz * ptr->cosph0;
                break;
            case EQUIT:
                *phi = asin(y * sinz / rh);
                y = cosz * rh;
                x *= sinz;
                break;
            case N_POLE:
                *phi = asin(cosz);
                y = -y;
                break;
            case S_POLE:
                *phi = -asin(cosz);
                break;
            }
            *lam = atan2(x, y);
        }

        return false;
    }
}