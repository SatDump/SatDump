#include "stereo.h"
#include <cmath>
#include "../wgs84.h"
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923 /* pi/2 */
#endif
#ifndef M_PI_4
#define M_PI_4 0.78539816339744830962 /* pi/4 */
#endif

/*
** This file was adapted and simplified from libproj, and the below
** notice kept as credits.
**
** libproj -- library of cartographic projections
**
** Copyright (c) 2004   Gerald I. Evenden
** Copyright (c) 2012   Martin Raspaud
**
** See also (section 4.4.3.2):
**   https://www.cgms-info.org/documents/pdf_cgms_03.pdf
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#define EPS10 1.e-10

namespace geodetic
{
    namespace projection
    {
        double pj_tsfn(double phi, double sinphi, double e)
        {
            double cosphi = cos(phi);
            return exp(e * atanh(e * sinphi)) * (sinphi > 0 ? cosphi / (1 + sinphi) : (1 - sinphi) / cosphi);
        }

        static double ssfn_(double phit, double sinphi, double eccen)
        {
            sinphi *= eccen;
            return (tan(.5 * (M_PI_2 + phit)) * pow((1. - sinphi) / (1. + sinphi), .5 * eccen));
        }

        int StereoProjection::init(double latitude, double longitude)
        {
            lon_0 = longitude; // The projection's longitude

            // Constants, WGS84
            e = WGS84::e;
            phi0 = latitude * 0.01745329;
            a = WGS84::a * 1000;
            es = WGS84::es;
            one_es = WGS84::one_es;

            if (es == 0.0)
            {
                // Illegal
                return 1;
            }

            k0 = .994;
            //x0 = 2000000.;
            //y0 = 2000000.;
            phits = M_PI_2;
            lam0 = 0.;

            // Setup
            double t;

            if (fabs((t = fabs(phi0)) - M_PI_2) < EPS10)
                mode = phi0 < 0. ? S_POLE : N_POLE;
            else
                mode = t > EPS10 ? OBLIQ : EQUIT;
            phits = fabs(phits);

            if (es != 0.0)
            {
                double X;

                switch (mode)
                {
                case N_POLE:
                case S_POLE:
                    if (fabs(phits - M_PI_2) < EPS10)
                        akm1 = 2. * k0 /
                               sqrt(pow(1 + e, 1 + e) * pow(1 - e, 1 - e));
                    else
                    {
                        t = sin(phits);
                        akm1 = cos(phits) / pj_tsfn(phits, t, e);
                        t *= e;
                        akm1 /= sqrt(1. - t * t);
                    }
                    break;
                case EQUIT:
                case OBLIQ:
                    t = sin(phi0);
                    X = 2. * atan(ssfn_(phi0, t, e)) - M_PI_2;
                    t *= e;
                    akm1 = 2. * k0 * cos(phi0) / sqrt(1. - t * t);
                    sinX1 = sin(X);
                    cosX1 = cos(X);
                    break;
                }
            }
            else
            {
                switch (mode)
                {
                case OBLIQ:
                    sinX1 = sin(phi0);
                    cosX1 = cos(phi0);
                    /*-fallthrough*/
                case EQUIT:
                    akm1 = 2. * k0;
                    break;
                case S_POLE:
                case N_POLE:
                    akm1 = fabs(phits - M_PI_2) >= EPS10 ? cos(phits) / tan(M_PI_4 - .5 * phits) : 2. * k0;
                    break;
                }
            }
            return 0;
        }

        int StereoProjection::forward(double lon, double lat, double &x, double &y)
        {
            x = y = 0; // Safety

            // Shift longitudes
            lon -= lon_0;
            if (lon < -180)
                lon = lon + 360;
            if (lon > 180)
                lon = lon - 360;

            // To radians
            double phi = lat * 0.01745329, lam = lon * 0.01745329;

            double coslam, sinlam, sinX = 0.0, cosX = 0.0, A = 0.0, sinphi;

            coslam = cos(lam);
            sinlam = sin(lam);
            sinphi = sin(phi);
            if (mode == OBLIQ || mode == EQUIT)
            {
                const double X = 2. * atan(ssfn_(phi, sinphi, e)) - M_PI_2;
                sinX = sin(X);
                cosX = cos(X);
            }

            switch (mode)
            {
            case OBLIQ:
            {
                const double denom = cosX1 * (1. + sinX1 * sinX + cosX1 * cosX * coslam);
                if (denom == 0)
                {
                    // Illegal
                    return 1;
                }
                A = akm1 / denom;
                y = A * (cosX1 * sinX - sinX1 * cosX * coslam);
                x = A * cosX;
                break;
            }

            case EQUIT:
                /* avoid zero division */
                if (1. + cosX * coslam == 0.0)
                {
                    y = HUGE_VAL;
                }
                else
                {
                    A = akm1 / (1. + cosX * coslam);
                    y = A * sinX;
                }
                x = A * cosX;
                break;

            case S_POLE:
                phi = -phi;
                coslam = -coslam;
                sinphi = -sinphi;
                /*-fallthrough*/
            case N_POLE:
                if (fabs(phi - M_PI_2) < 1e-15)
                    x = 0;
                else
                    x = akm1 * pj_tsfn(phi, sinphi, e);
                y = -x * coslam;
                break;
            }

            x = x * sinlam;
            return 0;
        }

        int StereoProjection::inverse(double x, double y, double &lon, double &lat)
        {
            lon = lat = 0.0;
            double phi = 0, lam = 0;

            double cosphi, sinphi, tp = 0.0, phi_l = 0.0, rho, halfe = 0.0, halfpi = 0.0;

            rho = hypot(x, y);

            switch (mode)
            {
            case OBLIQ:
            case EQUIT:
                tp = 2. * atan2(rho * cosX1, akm1);
                cosphi = cos(tp);
                sinphi = sin(tp);
                if (rho == 0.0)
                    phi_l = asin(cosphi * sinX1);
                else
                    phi_l = asin(cosphi * sinX1 + (y * sinphi * cosX1 / rho));

                tp = tan(.5 * (M_PI_2 + phi_l));
                x *= sinphi;
                y = rho * cosX1 * cosphi - y * sinX1 * sinphi;
                halfpi = M_PI_2;
                halfe = .5 * e;
                break;
            case N_POLE:
                y = -y;
                /*-fallthrough*/
            case S_POLE:
                tp = -rho / akm1;
                phi_l = M_PI_2 - 2. * atan(tp);
                halfpi = -M_PI_2;
                halfe = -.5 * e;
                break;
            }

            for (int i = 8; i > 0; --i)
            {
                sinphi = e * sin(phi_l);
                phi = 2. * atan(tp * pow((1. + sinphi) / (1. - sinphi), halfe)) - halfpi;
                if (fabs(phi_l - phi) < 1.e-10)
                {
                    if (mode == S_POLE)
                        phi = -phi;
                    lam = (x == 0. && y == 0.) ? 0. : atan2(x, y);

                    // To degs
                    lat = phi * 57.29578;
                    lon = lam * 57.29578;

                    // Shift longitudes back to reference 0
                    lon += lon_0;
                    if (lon < -180)
                        lon = lon + 360;
                    if (lon > 180)
                        lon = lon - 360;

                    return 0;
                }
                phi_l = phi;
            }

            return 1;
        }
    };
};