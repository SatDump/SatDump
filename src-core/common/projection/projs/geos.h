#pragma once

/*
Implementation of a standard GEOS projection, adapted from libproj.
Some variables are hardcoded for the intended usecase, making some
degree of tuning unecessary.
Uses the WGS84 ellipsoid.
*/
namespace geodetic
{
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
            GEOSProjection()
            {
                init(0, 0);
            }
            GEOSProjection(double height, double longitude, bool sweep_x = false)
            {
                init(height, longitude, sweep_x);
            }

            int init(double height, double longitude, bool sweep_x = false); // return value of 1 => Error
            int forward(double lon, double lat, double &x, double &y);       // return value of 1 => Error
            int inverse(double x, double y, double &lon, double &lat);       // return value of 1 => Error

            void get_for_gpu_float(float *v) // 13 values
            {
                v[0] = h;
                v[1] = radius_p;
                v[2] = radius_p2;
                v[3] = radius_p_inv2;
                v[4] = radius_g;
                v[5] = radius_g_1;
                v[6] = C;
                v[7] = flip_axis;
                v[8] = phi0;
                v[9] = a;
                v[10] = es;
                v[11] = one_es;
                v[12] = lon_0;
            }
        };
    };
};