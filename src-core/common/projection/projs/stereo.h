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
            double lam0;

            double lon_0;

        public:
            StereoProjection()
            {
                init(false, 0);
            }
            StereoProjection(double latitude, double longitude)
            {
                init(latitude, longitude);
            }

            int init(double latitude, double longitude);               // return value of 1 => Error
            int forward(double lon, double lat, double &x, double &y); // return value of 1 => Error
            int inverse(double x, double y, double &lon, double &lat); // return value of 1 => Error

            void get_for_gpu_float(float *v) // 13 values
            {
                v[0] = phits;
                v[1] = sinX1;
                v[2] = cosX1;
                v[3] = akm1;
                v[4] = mode;
                v[5] = e;
                v[6] = phi0;
                v[7] = a;
                v[8] = es;
                v[9] = one_es;
                v[10] = k0;
                v[11] = lam0;
                v[12] = lon_0;
            }
        };
    };
};