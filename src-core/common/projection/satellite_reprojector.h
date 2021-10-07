#pragma once
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include <functional>
#include "leo_projection.h"
#include "geo_projection.h"

/*
Implementation of a function capable of plotting LEO satellite image (or anything that
can be done with a LEOScanProjector) to an equirectangular projection for easier viewing.
This has some known defects right now such as leaving gaps in the image. It's something I
will have to fix at some point... Still thinking about the best approach :-)
*/
namespace projection
{
    // Reproject LEO imagery
    void reprojectLEOtoProj(
        cimg_library::CImg<unsigned short> image,                                     // Input image to project
        projection::LEOScanProjector &projector,                                      // LEO Projector
        cimg_library::CImg<unsigned char> &projected_image,                           // Optional input image
        int channels,                                                                 // Channels
        std::function<std::pair<int, int>(float, float, int, int)> projectionFunction // Optional projection function, default is equirectangular
    );

    // Reproject GEO imagery
    void reprojectGEOtoProj(
        cimg_library::CImg<unsigned short> image,                                     // Input image to project
        projection::GEOProjector &projector,                                          // GEO Projector
        cimg_library::CImg<unsigned char> &projected_image,                           // Optional input image
        int channels,                                                                 // Channels
        std::function<std::pair<int, int>(float, float, int, int)> projectionFunction // Optional projection function, default is equirectangular
    );

    void projectEQUIToproj(cimg_library::CImg<unsigned short> image,
                           cimg_library::CImg<unsigned char> &projected_image,
                           int channels,
                           std::function<std::pair<int, int>(float, float, int, int)> toMapCoords);

    // Reproject LEO imagery to an equirectangular projection
    cimg_library::CImg<unsigned char> projectLEOToEquirectangularMapped(
        cimg_library::CImg<unsigned short> image,                                                             // Input image to project
        projection::LEOScanProjector &projector,                                                              // LEO Projector
        int output_width,                                                                                     // Output map width
        int output_height,                                                                                    // Output map height
        int channels = 1,                                                                                     // Channel count
        cimg_library::CImg<unsigned char> projected_image = cimg_library::CImg<unsigned char>(1, 1, 1, 1, 0), // Optional input image
        std::function<std::pair<int, int>(float, float, int, int)> toMapCoords = [](float lat, float lon, int map_height, int map_width) -> std::pair<int, int>
        {
            int imageLat = map_height - ((90.0f + lat) / 180.0f) * map_height;
            int imageLon = (lon / 360.0f) * map_width + (map_width / 2);
            return {imageLon, imageLat};
        } // Optional projection function, default is equirectangular
    );
};