#include "equirectangular.h"
#include <cmath>

namespace satdump
{
    namespace projection
    {
        void EquirectangularProjection::init(int img_width, int img_height, float tl_lon, float tl_lat, float br_lon, float br_lat)
        {
            image_height = img_height;
            image_width = img_width;

            top_left_lat = tl_lat;
            top_left_lon = tl_lon;

            bottom_right_lat = br_lat;
            bottom_right_lon = br_lon;

            // Compute how much we cover on the input image
            covered_lat = fabs(top_left_lat - bottom_right_lat);
            covered_lon = fabs(top_left_lon - bottom_right_lon);

            // Compute the offset the top right corner has
            offset_lat = fabs(top_left_lat - 90);
            offset_lon = fabs(top_left_lon + 180);
        }

        void EquirectangularProjection::forward(float lon, float lat, int &x, int &y, bool allow_oob)
        {
            if ((lat > top_left_lat || lat < bottom_right_lat || lon < top_left_lon || lon > bottom_right_lon) && !allow_oob)
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

            if ((y < 0 || y >= image_height || x < 0 || x >= image_width) && !allow_oob)
                x = y = -1;
        }

        void EquirectangularProjection::reverse(int x, int y, float &lon, float &lat)
        {
            if (y < 0 || y >= image_height || x < 0 || x >= image_width)
            {
                lon = lat = -1;
                return;
            }

            lat = (y / (float)image_height) * covered_lat;
            lon = (x / (float)image_width) * covered_lon;

            lat += offset_lat;
            lon += offset_lon;

            lat = 180.0f - (lat + 90.0f);
            lon -= 180;

            if (lat > top_left_lat || lat < bottom_right_lat || lon < top_left_lon || lon > bottom_right_lon)
                lon = lat = -1;
        }
    }; // namespace projection
}; // namespace satdump