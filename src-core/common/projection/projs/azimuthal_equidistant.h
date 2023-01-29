#pragma once

namespace geodetic
{
    namespace projection
    {
        class AzimuthalEquidistantProjection
        {
        private:
            int image_height;
            int image_width;

            float center_lat;
            float center_lon;

            double center_phi;
            double center_lam;

        public:
            void init(int img_width, int img_height, float cen_lon, float cen_lat);
            void forward(float lon, float lat, int &x, int &y);
            void reverse(int x, int y, float &lon, float &lat);
        };
    };
};