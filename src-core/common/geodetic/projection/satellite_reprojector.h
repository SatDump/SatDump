#pragma once
#include "common/image/image.h"
#include <functional>
#include "leo_projection.h"
#include "geo_projection.h"

/*
Implementation of a function capable of plotting LEO satellite image (or anything that
can be done with a LEOScanProjector) to an equirectangular projection for easier viewing.
This has some known defects right now such as leaving gaps in the image. It's something I
will have to fix at some point... Still thinking about the best approach :-)
*/
namespace geodetic
{
    namespace projection
    {
        // Reproject LEO imagery
        void reprojectLEOtoProj(
            image::Image<uint8_t> image,                                                   // Input image to project
            projection::LEOScanProjector &projector,                                       // LEO Projector
            image::Image<uint8_t> &projected_image,                                        // Optional input image
            int channels,                                                                  // Channels
            std::function<std::pair<int, int>(float, float, int, int)> projectionFunction, // Optional projection function, default is equirectangular
            float opacity = 1.0f,                                                          // Optional opacity
            float *progress = nullptr                                                      // Optional progress value
        );

        // Reproject GEO imagery
        void reprojectGEOtoProj(
            image::Image<uint8_t> image,                                                   // Input image to project
            projection::GEOProjector &projector,                                           // GEO Projector
            image::Image<uint8_t> &projected_image,                                        // Optional input image
            int channels,                                                                  // Channels
            std::function<std::pair<int, int>(float, float, int, int)> projectionFunction, // Optional projection function, default is equirectangular
            float opacity = 1.0f,                                                          // Optional opacity
            float *progress = nullptr                                                      // Optional progress value
        );

        void projectEQUIToproj(image::Image<uint8_t> image,
                               image::Image<uint8_t> &projected_image,
                               int channels,
                               std::function<std::pair<int, int>(float, float, int, int)> toMapCoords,
                               float opacity = 1.0f,
                               float *progress = nullptr);

        // Reproject LEO imagery to an equirectangular projection
        image::Image<uint8_t> projectLEOToEquirectangularMapped(
            image::Image<uint8_t> image,                                            // Input image to project
            projection::LEOScanProjector &projector,                                // LEO Projector
            int output_width,                                                       // Output map width
            int output_height,                                                      // Output map height
            int channels = 1,                                                       // Channel count
            image::Image<uint8_t> projected_image = image::Image<uint8_t>(1, 1, 1), // Optional input image
            std::function<std::pair<int, int>(float, float, int, int)> toMapCoords = [](float lat, float lon, int map_height, int map_width) -> std::pair<int, int>
            {
                int imageLat = map_height - ((90.0f + lat) / 180.0f) * map_height;
                int imageLon = (lon / 360.0f) * map_width + (map_width / 2);
                return {imageLon, imageLat};
            } // Optional projection function, default is equirectangular
        );

        // Reproject LEO imagery to an equirectangular projection
        image::Image<uint8_t> projectLEOToEquirectangularMapped(
            image::Image<uint16_t> image,                                           // Input image to project
            projection::LEOScanProjector &projector,                                // LEO Projector
            int output_width,                                                       // Output map width
            int output_height,                                                      // Output map height
            int channels = 1,                                                       // Channel count
            image::Image<uint8_t> projected_image = image::Image<uint8_t>(1, 1, 1), // Optional input image
            std::function<std::pair<int, int>(float, float, int, int)> toMapCoords = [](float lat, float lon, int map_height, int map_width) -> std::pair<int, int>
            {
                int imageLat = map_height - ((90.0f + lat) / 180.0f) * map_height;
                int imageLon = (lon / 360.0f) * map_width + (map_width / 2);
                return {imageLon, imageLat};
            } // Optional projection function, default is equirectangular
        );
    };
};