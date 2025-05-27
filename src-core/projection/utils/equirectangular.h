#pragma once

/*
Implementation of a standard GEOS projection, adapted from libproj.
Some variables are hardcoded for the intended usecase, making some
degree of tuning unecessary.
Uses the WGS84 ellipsoid.
*/
namespace satdump
{
    namespace projection
    {
        class EquirectangularProjection
        {
        private:
            int image_height;
            int image_width;

            float top_left_lat;
            float top_left_lon;

            float bottom_right_lat;
            float bottom_right_lon;

            float covered_lat;
            float covered_lon;

            float offset_lat;
            float offset_lon;

        public:
            void init(int img_width, int img_height, float tl_lon, float tl_lat, float br_lon, float br_lat);
            void forward(float lon, float lat, int &x, int &y, bool allow_oob = false);
            void reverse(int x, int y, float &lon, float &lat);
        };
    };
};