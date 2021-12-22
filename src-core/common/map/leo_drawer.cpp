#include "leo_drawer.h"
#include "common/map/map_drawer.h"
#include "resources.h"

namespace map
{
    image::Image<uint8_t> drawMapToLEO(image::Image<uint8_t> image, geodetic::projection::LEOScanProjector &projector)
    {
        image.to_rgb(); // If WB, make RGB

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