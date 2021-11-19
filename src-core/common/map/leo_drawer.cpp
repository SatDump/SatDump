#include "leo_drawer.h"
#include "common/map/map_drawer.h"
#include "resources.h"

namespace map
{
    cimg_library::CImg<unsigned char> drawMapToLEO(cimg_library::CImg<unsigned char> image, geodetic::projection::LEOScanProjector &projector)
    {
        if (image.spectrum() == 1) // If WB, make RGB
        {
            cimg_library::CImg<unsigned char> rgb_image(image.width(), image.height(), 1, 3, 0);
            memcpy(&rgb_image[rgb_image.width() * rgb_image.height() * 0], &image[0], rgb_image.width() * rgb_image.height());
            memcpy(&rgb_image[rgb_image.width() * rgb_image.height() * 1], &image[0], rgb_image.width() * rgb_image.height());
            memcpy(&rgb_image[rgb_image.width() * rgb_image.height() * 2], &image[0], rgb_image.width() * rgb_image.height());
            image = rgb_image;
        }

        std::function<std::pair<int, int>(float, float, int, int)> projectionFunc;
        projectionFunc = [&projector](float lat, float lon, int map_height, int map_width) -> std::pair<int, int>
        {
            int x;
            int y;
            projector.forward({lat, lon, 0}, x, y);

            if (x < 0 || x > map_width)
                return {-1, -1};
            if (y < 0 || y > map_height)
                return {-1, -1};

            return {x, y};
        };

        unsigned char color[3] = {0, 255, 0};
        map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")}, image, color, projectionFunc, 60);

        unsigned char color2[3] = {255, 0, 0};
        map::drawProjectedCapitalsGeoJson({resources::getResourcePath("maps/ne_10m_populated_places_simple.json")}, image, color2, projectionFunc, 0.4);

        return image;
    }
};