#include "satellite_reprojector.h"
#include "logger.h"
#include "resources.h"
#include "common/map/map_drawer.h"

namespace geodetic
{
    namespace projection
    {
        void reprojectLEOtoProj(cimg_library::CImg<unsigned short> image,
                                projection::LEOScanProjector &projector,
                                cimg_library::CImg<unsigned char> &projected_image,
                                int channels,
                                std::function<std::pair<int, int>(float, float, int, int)> projectionFunction,
                                float opacity,
                                float *progress)
        {
            for (int currentScan = 0; currentScan < (int)image.height(); currentScan++)
            {
                // Now compute each pixel's lat / lon and plot it
                for (double px = 0; px < image.width() - 1; px += 1)
                {
                    geodetic_coords_t coords1, coords2;
                    int ret1 = projector.inverse(px, currentScan, coords1);
                    int ret2 = projector.inverse(px + 1, currentScan, coords2);

                    if (ret1 || ret2)
                        continue;

                    std::pair<float, float> map_cc1 = projectionFunction(coords1.lat, coords1.lon, projected_image.height(), projected_image.width());
                    std::pair<float, float> map_cc2 = projectionFunction(coords2.lat, coords2.lon, projected_image.height(), projected_image.width());

                    unsigned char color[3] = {0, 0, 0};
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

                    if (color[0] == 0 && color[1] == 0 && color[2] == 0) // Skip Black
                        continue;

                    // This seems to glitch out sometimes... Need to check
                    double maxSize = projected_image.width() / 100.0;
                    if (abs(map_cc1.first - map_cc2.first) < maxSize && abs(map_cc1.second - map_cc2.second) < maxSize)
                    {
                        double circle_radius = sqrt(pow(int(map_cc1.first - map_cc2.first), 2) + pow(int(map_cc1.second - map_cc2.second), 2));
                        projected_image.draw_circle(map_cc1.first, map_cc1.second, ceil(circle_radius), color, 0.4 * opacity);
                    }

                    if (progress != nullptr)
                        *progress = float(currentScan) / float(image.height());
                }
            }
        }

        void reprojectGEOtoProj(cimg_library::CImg<unsigned short> image,
                                projection::GEOProjector &projector,
                                cimg_library::CImg<unsigned char> &projected_image,
                                int channels,
                                std::function<std::pair<int, int>(float, float, int, int)> projectionFunction,
                                float opacity, float *progress)
        {
            for (double lat = -90; lat < 90; lat += 0.01)
            {
                for (double lon = -180; lon < 180; lon += 0.01)
                {
                    int x, y;
                    if (projector.forward(lon, lat, x, y))
                        continue;

                    std::pair<float, float> map_cc1 = projectionFunction(lat, lon, projected_image.height(), projected_image.width());

                    unsigned char color[3];
                    if (channels == 3)
                    {
                        color[0] = image[image.width() * image.height() * 0 + y * image.width() + int(x)] >> 8;
                        color[1] = image[image.width() * image.height() * 1 + y * image.width() + int(x)] >> 8;
                        color[2] = image[image.width() * image.height() * 2 + y * image.width() + int(x)] >> 8;
                    }
                    else
                    {
                        color[0] = image[y * image.width() + int(x)] >> 8;
                        color[1] = image[y * image.width() + int(x)] >> 8;
                        color[2] = image[y * image.width() + int(x)] >> 8;
                    }

                    if (color[0] == 0 && color[1] == 0 && color[2] == 0) // Skip Black
                        continue;

                    if (color[0] >= 253 && color[1] >= 253 && color[2] >= 253) // Skip Full white, as it's usually filler on GEO (eg, xRIT)
                        continue;

                    //logger->info(std::to_string(color[0]) + " " + std::to_string(color[1]) + " " + std::to_string(color[2]));

                    projected_image.draw_point(map_cc1.first, map_cc1.second, color, opacity);
                }

                if (progress != nullptr)
                    *progress = (lat + 90) / 180;
            }
        }

        void projectEQUIToproj(cimg_library::CImg<unsigned short> image, cimg_library::CImg<unsigned char> &projected_image, int channels, std::function<std::pair<int, int>(float, float, int, int)> toMapCoords, float opacity, float *progress)
        {
            for (double lat = -90; lat < 90; lat += 0.01)
            {
                for (double lon = -180; lon < 180; lon += 0.01)
                {
                    int x = (lon / 360.0f) * image.width() + (image.width() / 2);
                    int y = image.height() - ((90.0f + lat) / 180.0f) * image.height();

                    if (x >= image.width() || y >= image.height())
                        continue;

                    std::pair<float, float> map_cc1 = toMapCoords(lat, lon, projected_image.height(), projected_image.width());

                    unsigned char color[3];
                    if (channels == 3)
                    {
                        color[0] = image[image.width() * image.height() * 0 + y * image.width() + int(x)] >> 8;
                        color[1] = image[image.width() * image.height() * 1 + y * image.width() + int(x)] >> 8;
                        color[2] = image[image.width() * image.height() * 2 + y * image.width() + int(x)] >> 8;
                    }
                    else
                    {
                        color[0] = image[y * image.width() + int(x)] >> 8;
                        color[1] = image[y * image.width() + int(x)] >> 8;
                        color[2] = image[y * image.width() + int(x)] >> 8;
                    }

                    if (color[0] == 0 && color[1] == 0 && color[2] == 0) // Skip Black
                        continue;

                    projected_image.draw_point(map_cc1.first, map_cc1.second, color, opacity);
                }

                if (progress != nullptr)
                    *progress = (lat + 90) / 180;
            }
        }

        cimg_library::CImg<unsigned char> projectLEOToEquirectangularMapped(cimg_library::CImg<unsigned short> image,
                                                                            projection::LEOScanProjector &projector,
                                                                            int output_width,
                                                                            int output_height,
                                                                            int channels,
                                                                            cimg_library::CImg<unsigned char> projected_image,
                                                                            std::function<std::pair<int, int>(float, float, int, int)> toMapCoords

        )
        {
            // Output mapped data
            if (projected_image.width() == 1 && projected_image.height() == 1)
                projected_image = cimg_library::CImg<unsigned char>(output_width, output_height, 1, 3, 0);

            reprojectLEOtoProj(image, projector, projected_image, channels, toMapCoords);

            unsigned char color[3] = {0, 255, 0};
            map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                           projected_image,
                                           color,
                                           toMapCoords);

            return projected_image;
        }
    };
};