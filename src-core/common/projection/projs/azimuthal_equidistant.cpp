#include "azimuthal_equidistant.h"
#include <cmath>
#include <iostream>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif


namespace geodetic
{
    namespace projection
    {
        void AzimuthalEquidistantProjection::init(int img_width, int img_height, float cen_lon, float cen_lat)
        {
            image_width = img_width;
            image_height = img_height;

            center_lat = cen_lat;
            if (center_lat == 90)
                center_lat = 89.999;
            center_lon = cen_lon;

            center_phi = center_lat / 57.29578;
            center_lam = center_lon / 57.29578;
        }

        void AzimuthalEquidistantProjection::forward(float lon, float lat, int &x, int &y)
        {
            double c, k, phi = lat / 57.29578, lam = lon / 57.29578;
            c = acos(sin(center_phi) * sin(phi) + cos(center_phi) * cos(phi) * cos(lam - center_lam));
            k = c / sin(c);

            x = (k * cos(phi) * sin(lam - center_lam) * image_width) / (2 * M_PI) + image_width / 2;
            y = (k * (cos(center_phi) * sin(phi) - sin(center_phi) * cos(phi) * cos(lam - center_lam)) * image_height) / (2 * M_PI) + image_height / 2;
            y = image_height - y;
        }

        void AzimuthalEquidistantProjection::reverse(int x, int y, float &lon, float &lat)
        {
            // x = image_width - x;
            y = image_height - y;
            double px, py, c;

            px = (2.0 * M_PI * (double)x) / (double)image_width - M_PI;
            py = (2.0 * M_PI * (double)y) / (double)image_height - M_PI;
            c = sqrt(pow(px, 2) + pow(py, 2));

            if (c > M_PI)
            {
                lat = -1;
                lon = -1;
                return;
            }
            lat = 57.29578 * asin(cos(c) * sin(center_phi) + py * sin(c) * cos(center_phi) / c);

            if (center_lat == 90)
            {
                lon = (center_lam + atan2(-1 * px, py)) * 57.29578;
            }
            else if (center_lat == -90)
            {
                lon = (center_lam + atan2(px, py)) * 57.29578;
            }
            else
            {
                lon = (center_lam + atan2(px * sin(c), (c * cos(center_phi) * cos(c) - py * sin(center_phi) * sin(c)))) * 57.29578;
            }

            if (lon < -180)
                lon += 360;
            if (lon > 180)
                lon -= 360;
        }
    };
};