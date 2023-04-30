#pragma once
#include <chrono>

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
            AzimuthalEquidistantProjection()
            {
                init(0, 0, 0, 0);
            }
            AzimuthalEquidistantProjection(int img_width, int img_height, float cen_lon, float cen_lat)
            {
                init(img_width, img_height, cen_lon, cen_lat);
            }

            void init(int img_width, int img_height, float cen_lon, float cen_lat);
            void forward(float lon, float lat, int &x, int &y);
            void reverse(int x, int y, float &lon, float &lat);

            void get_for_gpu_float(float *v){
                v[0] = image_height;
                v[1] = image_width;
                v[2] = center_lat;
                v[3] = center_lon;
                v[4] = center_phi;
                v[5] = center_lam;
            }
        };
    };
};