#pragma once
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include "leo_projection.h"

/*
Implementation of a function capable of plotting LEO satellite image (or anything that
can be done with a LEOScanProjector) to an equirectangular projection for easier viewing.
This has some known defects right now such as leaving gaps in the image. It's something I
will have to fix at some point... Still thinking about the best approach :-)
*/
namespace projection
{
    // Reproject LEO imagery to an equirectangular projection
    cimg_library::CImg<unsigned char> projectLEOToEquirectangularMapped(cimg_library::CImg<unsigned short> image,                                                            // Input image to project
                                                                        projection::LEOScanProjector projector,                                                              // LEO Projector
                                                                        int output_width,                                                                                    // Output map width
                                                                        int output_height,                                                                                   // Output map height
                                                                        int channels = 1,                                                                                    // Channel count
                                                                        cimg_library::CImg<unsigned char> projected_image = cimg_library::CImg<unsigned char>(1, 1, 1, 1, 0) // Optional input image

    );
};