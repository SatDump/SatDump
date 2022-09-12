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

            scale_x = 0.5;
            scale_y = 0.15 * (90.0 / 85.06);
        }

        void MercatorProjection::forward(float lon, float lat, int &x, int &y)
        {
            if (lat > 85.06 || lat < -85.06)
            {
                x = y = -1;
                return;
            }

            float px = (lon / 180.0) * (image_width * scale_x);
            float py = asinh(tan(lat / 57.29578)) * (image_height * scale_y);

            x = px + (image_width / 2);
            y = image_height - (py + (image_height / 2));

            if (x < 0 || y < 0)
                x = y = -1;
            if (x >= image_width || y >= image_height)
                x = y = -1;
        }

        void MercatorProjection::reverse(int x, int y, float &lon, float &lat)
        {
            if (y < 0 || y >= image_height || x < 0 || x >= image_width)
            {
                lon = lat = -1;
                return;
            }

            double px = (x - double(image_width / 2));
            double py = (image_height - double(y)) - double(image_height / 2);

            lat = atan(sinh(py / (image_height * scale_y))) * 57.29578;
            lon = (px / (image_width * scale_x)) * 180;

            if (lat > 85.06 || lat < -85.06)
            {
                lon = lat = -1;
                return;
            }
        }
    };
};