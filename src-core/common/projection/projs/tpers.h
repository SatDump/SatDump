#pragma once

/*
Implementation of a titled perspective projection, for example useful to
project data from the point of a view of a LEO satellite.
This was adapted from libproj.
Uses the WGS84 ellipsoid.
*/
namespace geodetic
{
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

            int init(double altitude, double longitude, double latitude, double tilt, double azi); // return value of 1 => Error
            int forward(double lon, double lat, double &x, double &y);                             // return value of 1 => Error
            int inverse(double x, double y, double &lon, double &lat);                             // return value of 1 => Error

            void get_for_gpu_float(float *v) // 18 values
            {
                v[0] = height;
                v[1] = sinph0;
                v[2] = cosph0;
                v[3] = p;
                v[4] = rp;
                v[5] = pn1;
                v[6] = pfact;
                v[7] = h;
                v[8] = cg;
                v[9] = sg;
                v[10] = sw;
                v[11] = cw;
                v[12] = mode;
                v[13] = tilt;
                v[14] = phi0;
                v[15] = a;
                v[16] = es;
                v[17] = lon_0;
            }
        };
    };
};