#pragma once

/*
Implementation of a standard GEOS projection, adapted from libproj.
Some variables are hardcoded for the intended usecase, making some
degree of tuning unecessary.
Uses the WGS84 ellipsoid.
*/
namespace projection
{
    class StereoProjection
    {
    private:
        enum Mode
        {
            S_POLE = 0,
            N_POLE = 1,
            OBLIQ = 2,
            EQUIT = 3
        };

        double phits;
        double sinX1;
        double cosX1;
        double akm1;
        enum Mode mode;

        double e;
        double phi0;
        double a;
        double es;
        double one_es;

        double k0;
        //double x0;
        //double y0;
        double lam0;

        double lon_0;

    public:
        StereoProjection()
        {
            init(false, 0);
        }
        StereoProjection(double latitude, double longitude, double x_0 = 0, double y_0 = 0)
        {
            init(latitude, longitude, x_0, y_0);
        }

        int init(double latitude, double longitude, double x_0 = 0, double y_0 = 0); // return value of 1 => Error
        int forward(double lon, double lat, double &x, double &y);                   // return value of 1 => Error
        int inverse(double x, double y, double &lon, double &lat);                   // return value of 1 => Error
    };
};