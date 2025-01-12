#include "tmerc.h"

namespace proj
{
    namespace
    {
        enum TMercAlgo
        {
            AUTO, // Poder/Engsager if far from central meridian, otherwise
                  // Evenden/Snyder
            EVENDEN_SNYDER,
            PODER_ENGSAGER,
        };

        // Approximate: Evenden/Snyder
        struct EvendenSnyder
        {
            double esp;
            double ml0;
            double *en;
        };

        // More exact: Poder/Engsager
        struct PoderEngsager
        {
            double Qn;     /* Merid. quad., scaled to the projection */
            double Zb;     /* Radius vector in polar coord. systems  */
            double cgb[6]; /* Constants for Gauss -> Geo lat */
            double cbg[6]; /* Constants for Geo lat -> Gauss */
            double utg[6]; /* Constants for transv. merc. -> geo */
            double gtu[6]; /* Constants for geo -> transv. merc. */
        };

        struct projection_tmerc_t
        {
            TMercAlgo algo;
            bool is_approx;
            bool is_exact;
            EvendenSnyder approx;
            PoderEngsager exact;
        };

#define Lmax 6

        // Evaluation sum(p[i] * x^i, i, 0, N) via Horner's method.  N.B. p is of
        // length N+1.
        static double polyval(double x, const double p[], int N)
        {
            double y = N < 0 ? 0 : p[N];
            for (; N > 0;)
                y = y * x + p[--N];
            return y;
        }

        // Evaluate y = sum(c[k] * sin((2*k+2) * zeta), k, 0, K-1)
        static double clenshaw(double szeta, double czeta, const double c[], int K)
        {
            // Approx operation count = (K + 5) mult and (2 * K + 2) add
            double u0 = 0, u1 = 0,                         // accumulators for sum
                X = 2 * (czeta - szeta) * (czeta + szeta); // 2 * cos(2*zeta)
            for (; K > 0;)
            {
                double t = X * u0 - u1 + c[--K];
                u1 = u0;
                u0 = t;
            }
            return 2 * szeta * czeta * u0; // sin(2*zeta) * u0
        }

        double *pj_enfn(double n)
        {
            // Expansion of (quarter meridian) / ((a+b)/2 * pi/2) as series in n^2;
            // these coefficients are ( (2*k - 3)!! / (2*k)!! )^2 for k = 0..3
            static const double coeff_rad[] = {1, 1.0 / 4, 1.0 / 64, 1.0 / 256};

            // Coefficients to convert phi to mu, Eq. A5 in arXiv:2212.05818
            // with 0 terms dropped
            static const double coeff_mu_phi[] = {
                -3.0 / 2,
                9.0 / 16,
                -3.0 / 32,
                15.0 / 16,
                -15.0 / 32,
                135.0 / 2048,
                -35.0 / 48,
                105.0 / 256,
                315.0 / 512,
                -189.0 / 512,
                -693.0 / 1280,
                1001.0 / 2048,
            };
            // Coefficients to convert mu to phi, Eq. A6 in arXiv:2212.05818
            // with 0 terms dropped
            static const double coeff_phi_mu[] = {
                3.0 / 2,
                -27.0 / 32,
                269.0 / 512,
                21.0 / 16,
                -55.0 / 32,
                6759.0 / 4096,
                151.0 / 96,
                -417.0 / 128,
                1097.0 / 512,
                -15543.0 / 2560,
                8011.0 / 2560,
                293393.0 / 61440,
            };

            double n2 = n * n, d = n, *en;

            // 2*Lmax for the Fourier coeffs for each direction of conversion + 1 for
            // overall multiplier.
            en = (double *)malloc((2 * Lmax + 1) * sizeof(double));
            if (nullptr == en)
                return nullptr;
            en[0] = polyval(n2, coeff_rad, Lmax / 2) / (1 + n);
            for (int l = 0, o = 0; l < Lmax; ++l)
            {
                int m = (Lmax - l - 1) / 2;
                en[l + 1] = d * polyval(n2, coeff_mu_phi + o, m);
                en[l + 1 + Lmax] = d * polyval(n2, coeff_phi_mu + o, m);
                d *= n;
                o += m + 1;
            }
            return en;
        }

        double pj_mlfn(double phi, double sphi, double cphi, const double *en)
        {
            return en[0] * (phi + clenshaw(sphi, cphi, en + 1, Lmax));
        }

        double pj_inv_mlfn(double mu, const double *en)
        {
            mu /= en[0];
            return mu + clenshaw(sin(mu), cos(mu), en + 1 + Lmax, Lmax);
        }

        /* Helper functions for "exact" transverse mercator */
        inline static double gatg(const double *p1, int len_p1, double B, double cos_2B,
                                  double sin_2B)
        {
            double h = 0, h1, h2 = 0;

            const double two_cos_2B = 2 * cos_2B;
            const double *p = p1 + len_p1;
            h1 = *--p;
            while (p - p1)
            {
                h = -h2 + two_cos_2B * h1 + *--p;
                h2 = h1;
                h1 = h;
            }
            return (B + h * sin_2B);
        }

        /* Complex Clenshaw summation */
        inline static double clenS(const double *a, int size, double sin_arg_r,
                                   double cos_arg_r, double sinh_arg_i,
                                   double cosh_arg_i, double *R, double *I)
        {
            double r, i, hr, hr1, hr2, hi, hi1, hi2;

            /* arguments */
            const double *p = a + size;
            r = 2 * cos_arg_r * cosh_arg_i;
            i = -2 * sin_arg_r * sinh_arg_i;

            /* summation loop */
            hi1 = hr1 = hi = 0;
            hr = *--p;
            for (; a - p;)
            {
                hr2 = hr1;
                hi2 = hi1;
                hr1 = hr;
                hi1 = hi;
                hr = -hr2 + r * hr1 - i * hi1 + *--p;
                hi = -hi2 + i * hr1 + r * hi1;
            }

            r = sin_arg_r * cosh_arg_i;
            i = cos_arg_r * sinh_arg_i;
            *R = r * hr - i * hi;
            *I = r * hi + i * hr;
            return *R;
        }

        /* Real Clenshaw summation */
        static double clens(const double *a, int size, double arg_r)
        {
            double r, hr, hr1, hr2, cos_arg_r;

            const double *p = a + size;
            cos_arg_r = cos(arg_r);
            r = 2 * cos_arg_r;

            /* summation loop */
            hr1 = 0;
            hr = *--p;
            for (; a - p;)
            {
                hr2 = hr1;
                hr1 = hr;
                hr = -hr2 + r * hr1 + *--p;
            }
            return sin(arg_r) * hr;
        }

        double adjlon(double longitude)
        {
            /* Let longitude slightly overshoot, to avoid spurious sign switching at the
             * date line */
            if (fabs(longitude) < M_PI + 1e-12)
                return longitude;

            /* adjust to 0..2pi range */
            longitude += M_PI;

            /* remove integral # of 'revolutions'*/
            longitude -= M_TWOPI * floor(longitude / M_TWOPI);

            /* adjust back to -pi..pi range */
            longitude -= M_PI;

            return longitude;
        }
    }

    /* Constants for "approximate" transverse mercator */
#define EPS10 1.e-10
#define FC1 1.
#define FC2 .5
#define FC3 .16666666666666666666
#define FC4 .08333333333333333333
#define FC5 .05
#define FC6 .03333333333333333333
#define FC7 .02380952380952380952
#define FC8 .01785714285714285714

/* Constant for "exact" transverse mercator */
#define PROJ_ETMERC_ORDER 6

    bool projection_tmerc_setup(projection_t *proj, int zone, bool south)
    {
        projection_tmerc_t *ptr = (projection_tmerc_t *)malloc(sizeof(projection_tmerc_t));
        proj->proj_dat = ptr;

        if (proj->type == ProjType_UniversalTransverseMercator)
        {
            if (proj->es == 0)
                return true;

            proj->y0 = south ? 10000000.0 : 0.0;
            proj->x0 = 500000.0;

            if (zone != -1)
            {
                zone = zone - 1;
            }
            else /* nearest central meridian input */
            {
                zone = lround((floor((adjlon(proj->lam0) + M_PI) * 30. / M_PI)));
                if (zone < 0)
                    zone = 0;
                else if (zone >= 60)
                    zone = 59;
            }

            proj->params.zone = zone + 1;

            proj->lam0 = (zone + .5) * M_PI / 30. - M_PI;
            proj->k0 = 0.9996;
            proj->phi0 = 0.;
        }
        else
        {
            return true;
        }

        ptr->algo = PODER_ENGSAGER;
        // We haven't worked on the criterion on inverse transformation
        // when phi0 != 0 or if k0 is not close to 1 or for very oblate
        // ellipsoid (es > 0.1 is ~ rf < 200)
        if (ptr->algo == TMercAlgo::AUTO && (proj->es > 0.1 || proj->phi0 != 0 || fabs(proj->k0 - 1) > 0.01))
            ptr->algo = TMercAlgo::PODER_ENGSAGER;

        if (proj->es == 0)
            ptr->algo = TMercAlgo::EVENDEN_SNYDER;

        switch (ptr->algo)
        {
        case TMercAlgo::EVENDEN_SNYDER:
        {
            ptr->is_approx = true;
            ptr->is_exact = false;
            if (proj->es == 0)
            {
                return true;
            }
            else
            {
            }
            break;
        }

        case TMercAlgo::PODER_ENGSAGER:
        {
            ptr->is_approx = false;
            ptr->is_exact = true;
            break;
        }

        case TMercAlgo::AUTO:
        {
            ptr->is_approx = true;
            ptr->is_exact = true;
            return true;
        }
        }

        if (ptr->is_approx)
        {
            EvendenSnyder *lptr = &ptr->approx;

            if (proj->es != 0.0)
            {
                if (!(lptr->en = pj_enfn(proj->n)))
                    return true;

                lptr->ml0 = pj_mlfn(proj->phi0, sin(proj->phi0), cos(proj->phi0), lptr->en);
                lptr->esp = proj->es / (1. - proj->es);
            }
            else
            {
                lptr->esp = proj->k0;
                lptr->ml0 = .5 * lptr->esp;
            }
        }

        if (ptr->is_exact)
        {
            PoderEngsager *lptr = &ptr->exact;

            // assert(proj->es > 0);

            /* third flattening */
            const double n = proj->n;
            double np = n;

            /* COEF. OF TRIG SERIES GEO <-> GAUSS */
            /* cgb := Gaussian -> Geodetic, KW p190 - 191 (61) - (62) */
            /* cbg := Geodetic -> Gaussian, KW p186 - 187 (51) - (52) */
            /* PROJ_ETMERC_ORDER = 6th degree : Engsager and Poder: ICC2007 */

            lptr->cgb[0] = n * (2 + n * (-2 / 3.0 + n * (-2 + n * (116 / 45.0 + n * (26 / 45.0 + n * (-2854 / 675.0))))));
            lptr->cbg[0] = n * (-2 + n * (2 / 3.0 + n * (4 / 3.0 + n * (-82 / 45.0 + n * (32 / 45.0 + n * (4642 / 4725.0))))));
            np *= n;
            lptr->cgb[1] = np * (7 / 3.0 + n * (-8 / 5.0 + n * (-227 / 45.0 + n * (2704 / 315.0 + n * (2323 / 945.0)))));
            lptr->cbg[1] = np * (5 / 3.0 + n * (-16 / 15.0 + n * (-13 / 9.0 + n * (904 / 315.0 + n * (-1522 / 945.0)))));
            np *= n;
            /* n^5 coeff corrected from 1262/105 -> -1262/105 */
            lptr->cgb[2] = np * (56 / 15.0 + n * (-136 / 35.0 + n * (-1262 / 105.0 + n * (73814 / 2835.0))));
            lptr->cbg[2] = np * (-26 / 15.0 + n * (34 / 21.0 + n * (8 / 5.0 + n * (-12686 / 2835.0))));
            np *= n;
            /* n^5 coeff corrected from 322/35 -> 332/35 */
            lptr->cgb[3] = np * (4279 / 630.0 + n * (-332 / 35.0 + n * (-399572 / 14175.0)));
            lptr->cbg[3] = np * (1237 / 630.0 + n * (-12 / 5.0 + n * (-24832 / 14175.0)));
            np *= n;
            lptr->cgb[4] = np * (4174 / 315.0 + n * (-144838 / 6237.0));
            lptr->cbg[4] = np * (-734 / 315.0 + n * (109598 / 31185.0));
            np *= n;
            lptr->cgb[5] = np * (601676 / 22275.0);
            lptr->cbg[5] = np * (444337 / 155925.0);

            /* Constants of the projections */
            /* Transverse Mercator (UTM, ITM, etc) */
            np = n * n;
            /* Norm. mer. quad, K&W p.50 (96), p.19 (38b), p.5 (2) */
            lptr->Qn = proj->k0 / (1 + n) * (1 + np * (1 / 4.0 + np * (1 / 64.0 + np / 256.0)));
            /* coef of trig series */
            /* utg := ell. N, E -> sph. N, E,  KW p194 (65) */
            /* gtu := sph. N, E -> ell. N, E,  KW p196 (69) */
            lptr->utg[0] = n * (-0.5 + n * (2 / 3.0 + n * (-37 / 96.0 + n * (1 / 360.0 + n * (81 / 512.0 + n * (-96199 / 604800.0))))));
            lptr->gtu[0] = n * (0.5 + n * (-2 / 3.0 + n * (5 / 16.0 + n * (41 / 180.0 + n * (-127 / 288.0 + n * (7891 / 37800.0))))));
            lptr->utg[1] = np * (-1 / 48.0 + n * (-1 / 15.0 + n * (437 / 1440.0 + n * (-46 / 105.0 + n * (1118711 / 3870720.0)))));
            lptr->gtu[1] = np * (13 / 48.0 + n * (-3 / 5.0 + n * (557 / 1440.0 + n * (281 / 630.0 + n * (-1983433 / 1935360.0)))));
            np *= n;
            lptr->utg[2] = np * (-17 / 480.0 + n * (37 / 840.0 + n * (209 / 4480.0 + n * (-5569 / 90720.0))));
            lptr->gtu[2] = np * (61 / 240.0 + n * (-103 / 140.0 + n * (15061 / 26880.0 + n * (167603 / 181440.0))));
            np *= n;
            lptr->utg[3] = np * (-4397 / 161280.0 + n * (11 / 504.0 + n * (830251 / 7257600.0)));
            lptr->gtu[3] = np * (49561 / 161280.0 + n * (-179 / 168.0 + n * (6601661 / 7257600.0)));
            np *= n;
            lptr->utg[4] = np * (-4583 / 161280.0 + n * (108847 / 3991680.0));
            lptr->gtu[4] = np * (34729 / 80640.0 + n * (-3418889 / 1995840.0));
            np *= n;
            lptr->utg[5] = np * (-20648693 / 638668800.0);
            lptr->gtu[5] = np * (212378941 / 319334400.0);

            /* Gaussian latitude value of the origin latitude */
            const double Z = gatg(lptr->cbg, PROJ_ETMERC_ORDER, proj->phi0, cos(2 * proj->phi0), sin(2 * proj->phi0));

            /* Origin northing minus true northing at the origin latitude */
            /* i.e. true northing = N - proj->Zb                         */
            lptr->Zb = -lptr->Qn * (Z + clens(lptr->gtu, PROJ_ETMERC_ORDER, 2 * Z));
        }

        return false;
    }

    bool projection_tmerc_fwd(const projection_t *proj, double lam, double phi, double *x, double *y)
    {
        projection_tmerc_t *ptr = (projection_tmerc_t *)proj->proj_dat;

        if (ptr->algo == EVENDEN_SNYDER)
        {
            EvendenSnyder *lptr = &ptr->approx;

            double b, cosphi;

            cosphi = cos(phi);
            b = cosphi * sin(lam);
            if (fabs(fabs(b) - 1.) <= EPS10)
            {
                return true;
            }

            *x = lptr->ml0 * log((1. + b) / (1. - b));
            *y = cosphi * cos(lam) / sqrt(1. - b * b);

            b = fabs(*y);
            if (cosphi == 1 && (lam < -M_HALFPI || lam > M_HALFPI))
            {
                /* Helps to be able to roundtrip |longitudes| > 90 at lat=0 */
                /* We could also map to -M_PI ... */
                *y = M_PI;
            }
            else if (b >= 1.)
            {
                if ((b - 1.) > EPS10)
                {
                    return true;
                }
                else
                    *y = 0.;
            }
            else
                *y = acos(*y);

            if (phi < 0.)
                *y = -*y;
            *y = lptr->esp * (*y - proj->phi0);

            return false;
        }
        else if (ptr->algo == PODER_ENGSAGER)
        {
            PoderEngsager *lptr = &ptr->exact;

            /* ell. LAT, LNG -> Gaussian LAT, LNG */
            double Cn = gatg(lptr->cbg, PROJ_ETMERC_ORDER, phi, cos(2 * phi), sin(2 * phi));
            /* Gaussian LAT, LNG -> compl. sph. LAT */
            const double sin_Cn = sin(Cn);
            const double cos_Cn = cos(Cn);
            const double sin_Ce = sin(lam);
            const double cos_Ce = cos(lam);

            const double cos_Cn_cos_Ce = cos_Cn * cos_Ce;
            Cn = atan2(sin_Cn, cos_Cn_cos_Ce);

            const double inv_denom_tan_Ce = 1. / hypot(sin_Cn, cos_Cn_cos_Ce);
            const double tan_Ce = sin_Ce * cos_Cn * inv_denom_tan_Ce;
#if 0
    // Variant of the above: found not to be measurably faster
    const double sin_Ce_cos_Cn = sin_Ce*cos_Cn;
    const double denom = sqrt(1 - sin_Ce_cos_Cn * sin_Ce_cos_Cn);
    const double tan_Ce = sin_Ce_cos_Cn / denom;
#endif

            /* compl. sph. N, E -> ell. norm. N, E */
            double Ce = asinh(tan_Ce); /* Replaces: Ce  = log(tan(FORTPI + Ce*0.5)); */

            /*
             *  Non-optimized version:
             *  const double sin_arg_r  = sin(2*Cn);
             *  const double cos_arg_r  = cos(2*Cn);
             *
             *  Given:
             *      sin(2 * Cn) = 2 sin(Cn) cos(Cn)
             *          sin(atan(y)) = y / sqrt(1 + y^2)
             *          cos(atan(y)) = 1 / sqrt(1 + y^2)
             *      ==> sin(2 * Cn) = 2 tan_Cn / (1 + tan_Cn^2)
             *
             *      cos(2 * Cn) = 2cos^2(Cn) - 1
             *                  = 2 / (1 + tan_Cn^2) - 1
             */
            const double two_inv_denom_tan_Ce = 2 * inv_denom_tan_Ce;
            const double two_inv_denom_tan_Ce_square = two_inv_denom_tan_Ce * inv_denom_tan_Ce;
            const double tmp_r = cos_Cn_cos_Ce * two_inv_denom_tan_Ce_square;
            const double sin_arg_r = sin_Cn * tmp_r;
            const double cos_arg_r = cos_Cn_cos_Ce * tmp_r - 1;

            /*
             *  Non-optimized version:
             *  const double sinh_arg_i = sinh(2*Ce);
             *  const double cosh_arg_i = cosh(2*Ce);
             *
             *  Given
             *      sinh(2 * Ce) = 2 sinh(Ce) cosh(Ce)
             *          sinh(asinh(y)) = y
             *          cosh(asinh(y)) = sqrt(1 + y^2)
             *      ==> sinh(2 * Ce) = 2 tan_Ce sqrt(1 + tan_Ce^2)
             *
             *      cosh(2 * Ce) = 2cosh^2(Ce) - 1
             *                   = 2 * (1 + tan_Ce^2) - 1
             *
             * and 1+tan_Ce^2 = 1 + sin_Ce^2 * cos_Cn^2 / (sin_Cn^2 + cos_Cn^2 *
             * cos_Ce^2) = (sin_Cn^2 + cos_Cn^2 * cos_Ce^2 + sin_Ce^2 * cos_Cn^2) /
             * (sin_Cn^2 + cos_Cn^2 * cos_Ce^2) = 1. / (sin_Cn^2 + cos_Cn^2 * cos_Ce^2)
             * = inv_denom_tan_Ce^2
             *
             */
            const double sinh_arg_i = tan_Ce * two_inv_denom_tan_Ce;
            const double cosh_arg_i = two_inv_denom_tan_Ce_square - 1;

            double dCn, dCe;
            Cn += clenS(lptr->gtu, PROJ_ETMERC_ORDER, sin_arg_r, cos_arg_r, sinh_arg_i, cosh_arg_i, &dCn, &dCe);
            Ce += dCe;
            if (fabs(Ce) <= 2.623395162778)
            {
                *y = lptr->Qn * Cn + lptr->Zb; /* Northing */
                *x = lptr->Qn * Ce;            /* Easting  */
            }
            else
            {
                return true;
            }

            return false;
        }

        return true;
    }

    bool projection_tmerc_inv(const projection_t *proj, double x, double y, double *lam, double *phi)
    {
        projection_tmerc_t *ptr = (projection_tmerc_t *)proj->proj_dat;

        if (ptr->algo == EVENDEN_SNYDER)
        {
            EvendenSnyder *lptr = &ptr->approx;

            *phi = pj_inv_mlfn(lptr->ml0 + y / proj->k0, lptr->en);
            if (fabs(*phi) >= M_HALFPI)
            {
                *phi = y < 0. ? -M_HALFPI : M_HALFPI;
                *lam = 0.;
            }
            else
            {
                double sinphi = sin(*phi), cosphi = cos(*phi);
                double t = fabs(cosphi) > 1e-10 ? sinphi / cosphi : 0.;
                const double n = lptr->esp * cosphi * cosphi;
                double con = 1. - proj->es * sinphi * sinphi;
                const double d = x * sqrt(con) / proj->k0;
                con *= t;
                t *= t;
                const double ds = d * d;
                *phi -= (con * ds / (1. - proj->es)) * FC2 * (1. - ds * FC4 * (5. + t * (3. - 9. * n) + n * (1. - 4 * n) - ds * FC6 * (61. + t * (90. - 252. * n + 45. * t) + 46. * n - ds * FC8 * (1385. + t * (3633. + t * (4095. + 1575. * t))))));
                *lam = d * (FC1 - ds * FC3 * (1. + 2. * t + n - ds * FC5 * (5. + t * (28. + 24. * t + 8. * n) + 6. * n - ds * FC7 * (61. + t * (662. + t * (1320. + 720. * t)))))) / cosphi;
            }

            return false;
        }
        else if (ptr->algo == PODER_ENGSAGER)
        {
            PoderEngsager *lptr = &ptr->exact;

            /* normalize N, E */
            double Cn = (y - lptr->Zb) / lptr->Qn;
            double Ce = x / lptr->Qn;

            if (fabs(Ce) <= 2.623395162778)
            { /* 150 degrees */
                /* norm. N, E -> compl. sph. LAT, LNG */
                const double sin_arg_r = sin(2 * Cn);
                const double cos_arg_r = cos(2 * Cn);

                // const double sinh_arg_i = sinh(2*Ce);
                // const double cosh_arg_i = cosh(2*Ce);
                const double exp_2_Ce = exp(2 * Ce);
                const double half_inv_exp_2_Ce = 0.5 / exp_2_Ce;
                const double sinh_arg_i = 0.5 * exp_2_Ce - half_inv_exp_2_Ce;
                const double cosh_arg_i = 0.5 * exp_2_Ce + half_inv_exp_2_Ce;

                double dCn_ignored, dCe;
                Cn += clenS(lptr->utg, PROJ_ETMERC_ORDER, sin_arg_r, cos_arg_r, sinh_arg_i,
                            cosh_arg_i, &dCn_ignored, &dCe);
                Ce += dCe;

                /* compl. sph. LAT -> Gaussian LAT, LNG */
                const double sin_Cn = sin(Cn);
                const double cos_Cn = cos(Cn);

#if 0
        // Non-optimized version:
        double sin_Ce, cos_Ce;
        Ce = atan (sinh (Ce));  // Replaces: Ce = 2*(atan(exp(Ce)) - FORTPI);
        sin_Ce = sin (Ce);
        cos_Ce = cos (Ce);
        Ce     = atan2 (sin_Ce, cos_Ce*cos_Cn);
        Cn     = atan2 (sin_Cn*cos_Ce,  hypot (sin_Ce, cos_Ce*cos_Cn));
#else
                /*
                 *      One can divide both member of Ce = atan2(...) by cos_Ce, which
                 * gives: Ce     = atan2 (tan_Ce, cos_Cn) = atan2(sinh(Ce), cos_Cn)
                 *
                 *      and the same for Cn = atan2(...)
                 *      Cn     = atan2 (sin_Cn, hypot (sin_Ce, cos_Ce*cos_Cn)/cos_Ce)
                 *             = atan2 (sin_Cn, hypot (sin_Ce/cos_Ce, cos_Cn))
                 *             = atan2 (sin_Cn, hypot (tan_Ce, cos_Cn))
                 *             = atan2 (sin_Cn, hypot (sinhCe, cos_Cn))
                 */
                const double sinhCe = sinh(Ce);
                Ce = atan2(sinhCe, cos_Cn);
                const double modulus_Ce = hypot(sinhCe, cos_Cn);
                Cn = atan2(sin_Cn, modulus_Ce);
#endif

                /* Gaussian LAT, LNG -> ell. LAT, LNG */

                // Optimization of the computation of cos(2*Cn) and sin(2*Cn)
                const double tmp = 2 * modulus_Ce / (sinhCe * sinhCe + 1);
                const double sin_2_Cn = sin_Cn * tmp;
                const double cos_2_Cn = tmp * modulus_Ce - 1.;
                // const double cos_2_Cn = cos(2 * Cn);
                // const double sin_2_Cn = sin(2 * Cn);

                *phi = gatg(lptr->cgb, PROJ_ETMERC_ORDER, Cn, cos_2_Cn, sin_2_Cn);
                *lam = Ce;
            }
            else
            {
                return true;
            }

            return false;
        }

        return true;
    }
}