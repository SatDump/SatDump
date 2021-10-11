#include "vincentys_calculations.h"
#include "wgs84.h"

#define M_2PI (M_PI * 2)

/*
The code in this file was ported to C++ and slightly modified
from https://github.com/airbreather/Gavaghan.Geodesy/blob/master/Source/Gavaghan.Geodesy/GeodeticCalculator.cs

Original license :
---------------------------------------------------------------------------------------
Gavaghan.Geodesy by Mike Gavaghan

http://www.gavaghan.org/blog/free-source-code/geodesy-library-vincentys-formula/

This code may be freely used and modified on any personal or professional
project.  It comes with no warranty.

BitCoin tips graciously accepted at 1FB63FYQMy7hpC2ANVhZ5mSgAZEtY1aVLf
---------------------------------------------------------------------------------------
*/
namespace geodetic
{
    geodetic_coords_t vincentys_forward(geodetic_coords_t start, double initialBearing, double distance, double &finalBearing, double tolerance)
    {
        const double &a = WGS84::a;
        const double &b = WGS84::b;
        const double aSquared = a * a;
        const double bSquared = b * b;
        const double &f = WGS84::f;

        start.toRads(); // Ensure

        double phi1 = start.lat;
        double alpha1 = initialBearing;
        double cosAlpha1 = cos(alpha1);
        double sinAlpha1 = sin(alpha1);
        double s = distance * 1000; // To meters from Km
        double tanU1 = (1.0 - f) * tan(phi1);
        double cosU1 = 1.0 / sqrt(1.0 + tanU1 * tanU1);
        double sinU1 = tanU1 * cosU1;

        // eq. 1
        double sigma1 = atan2(tanU1, cosAlpha1);

        // eq. 2
        double sinAlpha = cosU1 * sinAlpha1;
        double sin2Alpha = sinAlpha * sinAlpha;
        double cos2Alpha = 1 - sin2Alpha;
        double uSquared = cos2Alpha * (aSquared - bSquared) / bSquared;

        // eq. 3
        double A = 1 + (uSquared / 16384) * (4096 + uSquared * (-768 + uSquared * (320 - 175 * uSquared)));

        // eq. 4
        double B = (uSquared / 1024) * (256 + uSquared * (-128 + uSquared * (74 - 47 * uSquared)));

        // iterate until there is a negligible change in sigma
        double deltaSigma;
        double sOverbA = s / (b * A);
        double sigma = sOverbA;
        double sinSigma;
        double prevSigma = sOverbA;
        double sigmaM2;
        double cosSigmaM2;
        double cos2SigmaM2;

        for (;;)
        {
            // eq. 5
            sigmaM2 = 2.0 * sigma1 + sigma;
            cosSigmaM2 = cos(sigmaM2);
            cos2SigmaM2 = cosSigmaM2 * cosSigmaM2;
            sinSigma = sin(sigma);
            double cosSignma = cos(sigma);

            // eq. 6
            deltaSigma = B * sinSigma * (cosSigmaM2 + (B / 4.0) * (cosSignma * (-1 + 2 * cos2SigmaM2) - (B / 6.0) * cosSigmaM2 * (-3 + 4 * sinSigma * sinSigma) * (-3 + 4 * cos2SigmaM2)));

            // eq. 7
            sigma = sOverbA + deltaSigma;

            // break after converging to tolerance
            if (abs(sigma - prevSigma) < tolerance)
                break;

            prevSigma = sigma;
        }

        sigmaM2 = 2.0 * sigma1 + sigma;
        cosSigmaM2 = cos(sigmaM2);
        cos2SigmaM2 = cosSigmaM2 * cosSigmaM2;

        double cosSigma = cos(sigma);
        sinSigma = sin(sigma);

        // eq. 8
        double sinU1sinSigma_cosU1cosSigmacosAlpha1 = sinU1 * sinSigma - cosU1 * cosSigma * cosAlpha1;
        double phi2 = atan2(sinU1 * cosSigma + cosU1 * sinSigma * cosAlpha1, (1.0 - f) * sqrt(sin2Alpha + (sinU1sinSigma_cosU1cosSigmacosAlpha1 * sinU1sinSigma_cosU1cosSigmacosAlpha1)));

        // eq. 9
        // This fixes the pole crossing defect spotted by Matt Feemster.  When a path
        // passes a pole and essentially crosses a line of latitude twice - once in
        // each direction - the longitude calculation got messed up.  Using Atan2
        // instead of Atan fixes the defect.  The change is in the next 3 lines.
        //double tanLambda = sinSigma * sinAlpha1 / (cosU1 * cosSigma - sinU1*sinSigma*cosAlpha1);
        //double lambda = atan(tanLambda);
        double lambda = atan2(sinSigma * sinAlpha1, cosU1 * cosSigma - sinU1 * sinSigma * cosAlpha1);

        // eq. 10
        double C = (f / 16) * cos2Alpha * (4 + f * (4 - 3 * cos2Alpha));

        // eq. 11
        double L = lambda - (1 - C) * f * sinAlpha * (sigma + C * sinSigma * (cosSigmaM2 + C * cosSigma * (-1 + 2 * cos2SigmaM2)));

        // eq. 12
        double alpha2 = atan2(sinAlpha, -sinU1 * sinSigma + cosU1 * cosSigma * cosAlpha1);

        // build result
        finalBearing = alpha2;

        return geodetic_coords_t(phi2, start.lon + L, start.alt, true);
    }

    geodetic_curve_t vincentys_inverse(geodetic_coords_t start, geodetic_coords_t end, double tolerance)
    {
        //
        // All equation numbers refer back to Vincenty's publication:
        // See http://www.ngs.noaa.gov/PUBS_LIB/inverse.pdf
        //

        // get constants
        const double &a = WGS84::a;
        const double &b = WGS84::b;
        const double &f = WGS84::f;

        start.toRads();
        end.toRads();

        // get parameters as radians
        double phi1 = start.lat;
        double lambda1 = start.lon;
        double phi2 = end.lat;
        double lambda2 = end.lon;

        // calculations
        double a2 = a * a;
        double b2 = b * b;
        double a2b2b2 = (a2 - b2) / b2;

        double omega = lambda2 - lambda1;

        double tanphi1 = tan(phi1);
        double tanU1 = (1.0 - f) * tanphi1;
        double U1 = atan(tanU1);
        double sinU1 = sin(U1);
        double cosU1 = cos(U1);

        double tanphi2 = tan(phi2);
        double tanU2 = (1.0 - f) * tanphi2;
        double U2 = atan(tanU2);
        double sinU2 = sin(U2);
        double cosU2 = cos(U2);

        double sinU1sinU2 = sinU1 * sinU2;
        double cosU1sinU2 = cosU1 * sinU2;
        double sinU1cosU2 = sinU1 * cosU2;
        double cosU1cosU2 = cosU1 * cosU2;

        // eq. 13
        double lambda = omega;

        // intermediates we'll need to compute 's'
        double A = 0.0;
        double B = 0.0;
        double sigma = 0.0;
        double deltasigma = 0.0;
        double lambda0;
        bool converged = false;

        for (int i = 0; i < 20; i++)
        {
            lambda0 = lambda;

            double sinlambda = sin(lambda);
            double coslambda = cos(lambda);

            // eq. 14
            double cosU1sinU2_sinU2cosU2coslambda = cosU1sinU2 - sinU1cosU2 * coslambda;
            double sin2sigma = (cosU2 * sinlambda * cosU2 * sinlambda) + (cosU1sinU2_sinU2cosU2coslambda * cosU1sinU2_sinU2cosU2coslambda);
            double sinsigma = sqrt(sin2sigma);

            // eq. 15
            double cossigma = sinU1sinU2 + (cosU1cosU2 * coslambda);

            // eq. 16
            sigma = atan2(sinsigma, cossigma);

            // eq. 17    Careful!  sin2sigma might be almost 0!
            double sinalpha = (sin2sigma == 0) ? 0.0 : cosU1cosU2 * sinlambda / sinsigma;
            double alpha = asin(sinalpha);
            double cosalpha = cos(alpha);
            double cos2alpha = cosalpha * cosalpha;

            // eq. 18    Careful!  cos2alpha might be almost 0!
            double cos2sigmam = cos2alpha == 0.0 ? 0.0 : cossigma - 2 * sinU1sinU2 / cos2alpha;
            double u2 = cos2alpha * a2b2b2;

            double cos2sigmam2 = cos2sigmam * cos2sigmam;

            // eq. 3
            A = 1.0 + u2 / 16384 * (4096 + u2 * (-768 + u2 * (320 - 175 * u2)));

            // eq. 4
            B = u2 / 1024 * (256 + u2 * (-128 + u2 * (74 - 47 * u2)));

            // eq. 6
            deltasigma = B * sinsigma * (cos2sigmam + B / 4 * (cossigma * (-1 + 2 * cos2sigmam2) - B / 6 * cos2sigmam * (-3 + 4 * sin2sigma) * (-3 + 4 * cos2sigmam2)));

            // eq. 10
            double C = f / 16 * cos2alpha * (4 + f * (4 - 3 * cos2alpha));

            // eq. 11 (modified)
            lambda = omega + (1 - C) * f * sinalpha * (sigma + C * sinsigma * (cos2sigmam + C * cossigma * (-1 + 2 * cos2sigmam2)));

            if (i < 2)
                continue;

            // see how much improvement we got
            double change = abs((lambda - lambda0) / lambda);

            if (change < tolerance)
            {
                converged = true;
                break;
            }
        }

        // eq. 19
        double s = b * A * (sigma - deltasigma);
        double alpha1 = 0;
        double alpha2 = 0;

        // didn't converge?  must be N/S
        if (!converged)
        {
            if (phi1 > phi2)
            {
                alpha1 = 180 * DEG_TO_RAD;
                alpha2 = 0 * DEG_TO_RAD;
            }
            else if (phi1 < phi2)
            {
                alpha1 = 0 * DEG_TO_RAD;
                alpha2 = 180 * DEG_TO_RAD;
            }
            else
            {
                //alpha1 = Angle.NaN;
                //alpha2 = Angle.NaN;
                //logger->error("Error");
            }
        }
        else
        {
            // eq. 20
            alpha1 = atan2(cosU2 * sin(lambda), (cosU1sinU2 - sinU1cosU2 * cos(lambda)));
            if (alpha1 < 0.0)
                alpha1 += M_2PI;

            // eq. 21
            alpha2 = atan2(cosU1 * sin(lambda), (-sinU1cosU2 + cosU1sinU2 * cos(lambda))) + M_PI;
            if (alpha2 < 0.0)
                alpha2 += M_2PI;
        }

        if (alpha1 >= M_2PI)
            alpha1 = alpha1 - M_2PI;
        if (alpha2 >= M_2PI)
            alpha2 = alpha2 - M_2PI;

        return geodetic_curve_t(s / 1000, alpha1, alpha2, true);
    }
};