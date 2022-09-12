#pragma once

#include "../projs/geos.h"

/*
Code to reference a decoded image (or similar data) from a GEO satellite to Lat / Lon coordinates.

The inverse function is currently broken, for an unknwon reason.
*/
namespace geodetic
{
    namespace projection
    {
        class GEOProjector
        {
        private:
            projection::GEOSProjection pj;
            double height, width;

            double x, y;
            double image_x, image_y;

            double hscale;
            double vscale;
            double x_offset;
            double y_offset;

        public:
            GEOProjector(double sat_lon,
                         double sat_height,
                         int img_width,
                         int img_height,
                         double hscale,
                         double vscale,
                         double x_offset,
                         double y_offset,
                         bool sweep_x);
            int forward(double lon, double lat, int &img_x, int &img_y);
            int inverse(int img_x, int img_y, double &lon, double &lat);

            void get_for_gpu_float(float *v) // 17 values
            {
                pj.get_for_gpu_float(v);
                v[13] = hscale;
                v[14] = vscale;
                v[15] = x_offset;
                v[16] = y_offset;
            }
        };
    };
};