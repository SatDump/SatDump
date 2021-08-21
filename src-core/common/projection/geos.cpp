#include "geos.h"
#include <cmath>

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

namespace projection
{
    void GEOSProjection::init(double height, double longitude, bool sweep_x )
    {
        lon_0 = longitude; // The satellite's longitude

        // Constants, extracted from Proj
        phi0 = 0;
        a = 6.37814e+06;
        es = 0.00669438;
        one_es = 0.993306;

        // Orbit Height
        h = height;

        // Scan axis
        flip_axis = sweep_x;

        radius_g_1 = h / a;
        if (radius_g_1 <= 0 || radius_g_1 > 1e10)
        {
            // Illegal case. 
            // Kept just in case but we shouldn't end up there unless the user makes a mistake...
        }

        // Init the rest
        radius_g = 1. + radius_g_1;
        C = radius_g * radius_g - 1.0;

        radius_p = sqrt(one_es);
        radius_p2 = one_es;
        radius_p_inv2 = one_es;
    }

    void GEOSProjection::forward(double lon, double lat, double &x, double &y)
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

        double r, Vx, Vy, Vz, tmp;

        // Calculation of geocentric latitude.
        phi = atan(radius_p2 * tan(phi));

        // Calculation of the three components of the vector from satellite to position on earth surface (lon,lat).
        r = (radius_p) / hypot(radius_p * cos(phi), sin(phi));
        Vx = r * cos(lam) * cos(phi);
        Vy = r * sin(lam) * cos(phi);
        Vz = r * sin(phi);

        // Check visibility.
        if (((radius_g - Vx) * Vx - Vy * Vy - Vz * Vz * radius_p_inv2) < 0.)
        {
            x = y = 2e10; // Trigger error
            return;
        }

        // Calculation based on view angles from satellite.
        tmp = radius_g - Vx;

        if (flip_axis)
        {
            x = radius_g_1 * atan(Vy / hypot(Vz, tmp));
            y = radius_g_1 * atan(Vz / tmp);
        }
        else
        {
            x = radius_g_1 * atan(Vy / tmp);
            y = radius_g_1 * atan(Vz / hypot(Vy, tmp));
        }
    }

    void GEOSProjection::inverse(double x, double y, double &lon, double &lat)
    {
        lon = lat = 0.0;
        double phi = 0, lam = 0;

        double Vx, Vy, Vz, a, b, k;

        // Setting three components of vector from satellite to position.
        Vx = -1.0;

        if (flip_axis)
        {
            Vz = tan(y / radius_g_1);
            Vy = tan(x / radius_g_1) * hypot(1.0, Vz);
        }
        else
        {
            Vy = tan(x / radius_g_1);
            Vz = tan(y / radius_g_1) * hypot(1.0, Vy);
        }

        // Calculation of terms in cubic equation and determinant.
        a = Vz / radius_p;
        a = Vy * Vy + a * a + Vx * Vx;
        b = 2 * radius_g * Vx;
        const double det = (b * b) - 4 * a * C;
        if (det < 0.0)
        {
            lon = lat = 2e10; // Trigger error
            return;
        }

        // Calculation of three components of vector from satellite to position.
        k = (-b - sqrt(det)) / (2. * a);
        Vx = radius_g + k * Vx;
        Vy *= k;
        Vz *= k;

        // Calculation of longitude and latitude.
        lam = atan2(Vy, Vx);
        phi = atan(Vz * cos(lam) / Vx);
        phi = atan(radius_p_inv2 * tan(phi));

        // To degs
        lat = phi * 57.29578;
        lon = lam * 57.29578;
    }
};