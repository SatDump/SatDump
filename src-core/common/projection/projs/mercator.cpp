#include "mercator.h"
#include <cmath>
#include "logger.h"

namespace geodetic
{
    namespace projection
    {
        void MercatorProjection::init(int img_width, int img_height /*, float tl_lon, float tl_lat, float br_lon, float br_lat*/)
        {
            scale_x = 0.5;
            scale_y = 0.15 * (90.0 / 85.06);

#if 0
            // Setup Crop
            if (true)
            {
                double tl_lon = 0;
                double tl_lat = 85.06;
                double br_lon = 180; // 180;
                double br_lat = 0;

                image_height = img_height * 100000;
                image_width = img_width * 100000;

                actual_image_height = image_height + 100;
                actual_image_width = image_width + 100;

                int x1, x2;
                int y1, y2;

                forward(tl_lon, tl_lat, x1, y1);
                forward(br_lon, br_lat, x2, y2);

                logger->critical("{:d} {:d} {:d} {:d}", x1, x2, y1, y2);

                double ratio_x = image_width / abs(x1 - x2);
                double ratio_y = image_height / abs(y1 - y2);

                offset_x = -x2 * ((double)img_width / (double)image_width);
                offset_y = y1 * ((double)img_height / (double)image_height);

                image_height = img_height * ratio_y;
                image_width = img_width * ratio_x;

                logger->critical("{:d} {:d}              {:d} {:d}", offset_x, offset_y, (img_width / image_width), (img_height / image_height));
            }
            else
            {
            }
#endif

            image_height = img_height;
            image_width = img_width;

            actual_image_height = img_height;
            actual_image_width = img_width;
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

            // x += offset_x;
            // y += offset_y;

            if (x < 0 || y < 0)
                x = y = -1;
            if (x >= actual_image_width || y >= actual_image_height)
                x = y = -1;
        }

        void MercatorProjection::reverse(int x, int y, float &lon, float &lat)
        {
            if (y < 0 || y >= actual_image_height || x < 0 || x >= actual_image_width)
            {
                lon = lat = -1;
                return;
            }

            double px = (x - double(image_width / 2));
            double py = (image_height - double(y)) - double(image_height / 2);

            lat = atan(sinh(py / (image_height * scale_y))) * 57.29578;
            lon = (px / (image_width * scale_x)) * 180;

            if (lat > 85.06 || lat < -85.06 || lon < -180 || lon > 180)
            {
                lon = lat = -1;
                return;
            }
        }
    };
};