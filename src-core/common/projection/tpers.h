#pragma once

/*
Implementation of a titled perspective projection, for example useful to
project data from the point of a view of a LEO satellite.
This was adapted from libproj.
Uses the WGS84 ellipsoid.
*/
namespace projection
{
    class TPERSProjection
    {
    private:
        enum Mode
        {
            N_POLE = 0,
            S_POLE = 1,
            EQUIT = 2,
            OBLIQ = 3
        };

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

        double phi0;
        double a;
        double es;

        double lon_0;

    public:
        TPERSProjection()
        {
            init(0, 0, 0, 0, 0);
        }
        TPERSProjection(double altitude, double longitude, double latitude, double tilt, double azi)
        {
            init(altitude, longitude, latitude, tilt, azi);
        }

        void init(double altitude, double longitude, double latitude, double tilt, double azi);
        void forward(double lon, double lat, double &x, double &y);
        void inverse(double x, double y, double &lon, double &lat);
    };
};