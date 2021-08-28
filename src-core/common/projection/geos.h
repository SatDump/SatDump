#pragma once

/*
Implementation of a standard GEOS projection, adapted from libproj.
Some variables are hardcoded for the intended usecase, making some
degree of tuning unecessary.
Uses the WGS84 ellipsoid.
*/
namespace projection
{
    class GEOSProjection
    {
    private:
        double h;
        double radius_p;
        double radius_p2;
        double radius_p_inv2;
        double radius_g;
        double radius_g_1;
        double C;
        int flip_axis;

        double phi0;
        double a;
        double es;
        double one_es;

        double lon_0;

    public:
        GEOSProjection(double height, double longitude, bool sweep_x = false)
        {
            init(height, longitude, sweep_x);
        }

        void init(double height, double longitude, bool sweep_x = false);
        void forward(double lon, double lat, double &x, double &y);
        void inverse(double x, double y, double &lon, double &lat);
    };
};