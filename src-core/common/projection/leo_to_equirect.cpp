#include "leo_to_equirect.h"
#include "logger.h"
#include "resources.h"
#include "common/map/map_drawer.h"

namespace projection
{
    cimg_library::CImg<unsigned char> projectLEOToEquirectangularMapped(cimg_library::CImg<unsigned short> image,
                                                                        projection::LEOScanProjector projector,
                                                                        int output_width,
                                                                        int output_height,
                                                                        int channels

    )
    {
        // Equirectangular proj
        std::function<std::pair<int, int>(float, float, int, int)> toMapCoords = [](float lat, float lon, int map_height, int map_width) -> std::pair<int, int>
        {
            int imageLat = map_height - ((90.0f + lat) / 180.0f) * map_height;
            int imageLon = (lon / 360.0f) * map_width + (map_width / 2);
            return {imageLon, imageLat};
        };

        // Output mapped data
        cimg_library::CImg<unsigned char> projected_image = cimg_library::CImg<unsigned char>(output_width, output_height, 1, 3, 0);

        // Reproject
        for (int currentScan = 0; currentScan < image.height(); currentScan++)
        {
            // Now compute each pixel's lat / lon and plot it
            for (double px = 0; px < image.width() - 1; px++)
            {
                double lat1, lon1, lat2, lon2;
                projector.inverse(px, currentScan, lat1, lon1);
                projector.inverse(px + 1, currentScan, lat2, lon2);

                if (fabs(lat1) > 1e10f || fabs(lon1) > 1e10f || fabs(lat2) > 1e10f || fabs(lon2) > 1e10f)
                    continue;

                std::pair<float, float> map_cc1 = toMapCoords(lat1, lon1, projected_image.height(), projected_image.width());
                std::pair<float, float> map_cc2 = toMapCoords(lat2, lon2, projected_image.height(), projected_image.width());

                unsigned char color[3];
                if (channels == 3)
                {
                    color[0] = image[image.width() * image.height() * 0 + currentScan * image.width() + int(px)] >> 8;
                    color[1] = image[image.width() * image.height() * 1 + currentScan * image.width() + int(px)] >> 8;
                    color[2] = image[image.width() * image.height() * 2 + currentScan * image.width() + int(px)] >> 8;
                }
                else
                {
                    color[0] = image[currentScan * image.width() + int(px)] >> 8;
                    color[1] = image[currentScan * image.width() + int(px)] >> 8;
                    color[2] = image[currentScan * image.width() + int(px)] >> 8;
                }

                if (map_cc1.first == map_cc2.first && map_cc1.second == map_cc2.second)
                    projected_image.draw_point(map_cc1.first, map_cc1.second, color);
                else if (abs(map_cc1.first - map_cc2.first) < 20 && abs(map_cc1.second - map_cc2.second) < 20)
                    projected_image.draw_rectangle(map_cc1.first, map_cc1.second, map_cc2.first, map_cc2.second, color);
            }

            //logger->critical(std::to_string(currentScan));
        }

        unsigned char color[3] = {0, 255, 0};

        map::drawProjectedMap(projected_image,
                              {resources::getResourcePath("maps/ne_10m_admin_0_countries.json")},
                              color,
                              [&toMapCoords](float lat, float lon, int height, int width) -> std::pair<int, int>
                              {
                                  std::pair<float, float> map_cc = toMapCoords(lat, lon, height, width);
                                  return {map_cc.first, map_cc.second};
                              });

        return projected_image;
    }
};