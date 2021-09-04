#include "tpers.h"
#include <cmath>
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923 /* pi/2 */
#endif

/*
** This file was adapted and simplified from libproj, and the below
** notice kept as credits.
**
** libproj -- library of cartographic projections
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

namespace projection
{
    void TPERSProjection::init(double altitude, double longitude, double latitude, double tilt, double azi)
    {
        lon_0 = longitude;

        double omega, gamma;
        omega = tilt * 0.01745329; // to rads
        gamma = azi * 0.01745329;  // to rads
        this->tilt = 1;
        cg = cos(gamma);
        sg = sin(gamma);
        cw = cos(omega);
        sw = sin(omega);

        height = altitude;
        phi0 = latitude * 0.01745329;
        a = 6.37814e+06;

        if (fabs(fabs(phi0) - M_PI_2) < EPS10)
            mode = phi0 < 0. ? S_POLE : N_POLE;
        else if (fabs(phi0) < EPS10)
            mode = EQUIT;
        else
        {
            mode = OBLIQ;
            sinph0 = sin(phi0);
            cosph0 = cos(phi0);
        }

        pn1 = height / a; // normalize by radius

        if (pn1 <= 0 || pn1 > 1e10)
        {
            // Illegal!!
        }

        p = 1. + pn1;
        rp = 1. / p;
        h = 1. / pn1;
        pfact = (p + 1.) * h;
        es = 0.;
    }

    void TPERSProjection::forward(double lon, double lat, double &x, double &y)
    {
        x = y = 0; // Safety

        // Shift longitudes to use the sat's as a reference
        lon -= lon_0;
        if (lon < -180)
            lon = lon + 360;
        if (lon > 180)
            lon = lon - 360;

        // To radians
        double phi = lat * 0.01745329, lam = lon * 0.01745329;

        double coslam, cosphi, sinphi;

        sinphi = sin(phi);
        cosphi = cos(phi);
        coslam = cos(lam);

        switch (mode)
        {
        case OBLIQ:
            y = sinph0 * sinphi + cosph0 * cosphi * coslam;
            break;
        case EQUIT:
            y = cosphi * coslam;
            break;
        case S_POLE:
            y = -sinphi;
            break;
        case N_POLE:
            y = sinphi;
            break;
        }

        if (y < rp)
        {
            x = y = 2e10; // Trigger error
            return;
        }

        y = pn1 / (p - y);
        x = y * cosphi * sin(lam);

        switch (mode)
        {
        case OBLIQ:
            y *= (cosph0 * sinphi -
                  sinph0 * cosphi * coslam);
            break;
        case EQUIT:
            y *= sinphi;
            break;
        case N_POLE:
            coslam = -coslam;
            /*-fallthrough*/
        case S_POLE:
            y *= cosphi * coslam;
            break;
        }

        if (tilt)
        {
            double yt, ba;

            yt = y * cg + x * sg;
            ba = 1. / (yt * sw * h + cw);
            x = (x * cg - y * sg) * cw * ba;
            y = yt * ba;
        }
    }

    void TPERSProjection::inverse(double x, double y, double &lon, double &lat)
    {
        lon = lat = 0.0;

        double phi = 0, lam = 0;
        double rh;

        if (tilt)
        {
            double bm, bq, yt;

            yt = 1. / (pn1 - y * sw);
            bm = pn1 * x * yt;
            bq = pn1 * y * cw * yt;
            x = bm * cg + bq * sg;
            y = bq * cg - bm * sg;
        }

        rh = hypot(x, y);

        if (fabs(rh) <= EPS10)
        {
            lam = 0.;
            phi = phi0;
        }
        else
        {
            double cosz, sinz;
            sinz = 1. - rh * rh * pfact;

            if (sinz < 0.)
            {
                // Illegal
                lon = lat = 2e10; // Trigger error
                return;
            }

            sinz = (p - sqrt(sinz)) / (pn1 / rh + rh / pn1);
            cosz = sqrt(1. - sinz * sinz);

            switch (mode)
            {
            case OBLIQ:
                phi = asin(cosz * sinph0 + y * sinz * cosph0 / rh);
                y = (cosz - sinph0 * sin(phi)) * rh;
                x *= sinz * cosph0;
                break;
            case EQUIT:
                phi = asin(y * sinz / rh);
                y = cosz * rh;
                x *= sinz;
                break;
            case N_POLE:
                phi = asin(cosz);
                y = -y;
                break;
            case S_POLE:
                phi = -asin(cosz);
                break;
            }

            lam = atan2(x, y);
        }

        // To degs
        lat = phi * 57.29578;
        lon = lam * 57.29578;

        // Shift longitudes back to reference 0
        lon += lon_0;
        if (lon < -180)
            lon = lon + 360;
        if (lon > 180)
            lon = lon - 360;
    }
};