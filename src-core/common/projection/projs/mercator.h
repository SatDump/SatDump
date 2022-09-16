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
        class MercatorProjection
        {
        private:
            int image_height;
            int image_width;

            int actual_image_height;
            int actual_image_width;

            // float top_left_lat;
            // float top_left_lon;

            // float bottom_right_lat;
            // float bottom_right_lon;

            // float covered_lat;
            // float covered_lon;

            // float offset_lat;
            // float offset_lon;

            double scale_x = 0;
            double scale_y = 0;

            // int offset_x = 0;
            // int offset_y = 0;

        public:
            void init(int img_width, int img_height /*, float tl_lon, float tl_lat, float br_lon, float br_lat*/);
            void forward(float lon, float lat, int &x, int &y);
            void reverse(int x, int y, float &lon, float &lat);

            void get_for_gpu_float(float *v) // 6 values
            {
                v[0] = image_height;
                v[1] = image_width;
                v[2] = actual_image_height;
                v[3] = actual_image_width;
                v[4] = scale_x;
                v[5] = scale_y;
            }
        };
    };
};