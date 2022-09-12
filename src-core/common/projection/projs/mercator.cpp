#include "mercator.h"
#include <cmath>

namespace geodetic
{
    namespace projection
    {
        void MercatorProjection::init(int img_width, int img_height /*, float tl_lon, float tl_lat, float br_lon, float br_lat*/)
        {
            image_height = img_height;
            image_width = img_width;

            //  top_left_lat = tl_lat;
            // top_left_lon = tl_lon;

            //   bottom_right_lat = br_lat;
            //   bottom_right_lon = br_lon;

            // Compute how much we cover on the input image
            //  covered_lat = fabs(top_left_lat - bottom_right_lat);
            //  covered_lon = fabs(top_left_lon - bottom_right_lon);

            // Compute the offset the top right corner has
            //  offset_lat = fabs(top_left_lat - 90);
            // offset_lon = fabs(top_left_lon + 180);

            scale = 100;
        }

        void MercatorProjection::forward(float lon, float lat, int &x, int &y)
        {
#if 0
            if (lat > top_left_lat || lat < bottom_right_lat || lon < top_left_lon || lon > bottom_right_lon)
            {
                x = y = -1;
                return;
            }

            lat = 180.0f - (lat + 90.0f);
            lon += 180;

            lat -= offset_lat;
            lon -= offset_lon;

            y = (lat / covered_lat) * image_height;
            x = (lon / covered_lon) * image_width;

            if (y < 0 || y >= image_height || x < 0 || x >= image_width)
                x = y = -1;
#endif
        }

        void MercatorProjection::reverse(int x, int y, float &lon, float &lat)
        {
#if 0
            if (y < 0 || y >= image_height || x < 0 || x >= image_width)
            {
                lon = lat = -1;
                return;
            }

            double phi = atan(pj_sinhpsi2tanphi(sinh(y / scale /*/ P->k0*/), WGS84::e));
            double lam = x / scale /*/ P->k0*/;

            // lat = (y / (float)image_height) * covered_lat;
            // lon = (x / (float)image_width) * covered_lon;

            lat = phi * 57.29578;
            lon = lam * 57.29578;

            lat = 180.0f - (lat + 90.0f);
            lon -= 180;

            // if (lat > top_left_lat || lat < bottom_right_lat || lon < top_left_lon || lon > bottom_right_lon)
            //     lon = lat = -1;
#endif
        }
    };
};