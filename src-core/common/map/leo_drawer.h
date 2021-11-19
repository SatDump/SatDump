#pragma once

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include <functional>
#include <vector>
#include "common/geodetic/projection/leo_projection.h"

namespace map
{
    /*
    Helper function to draw a map overlay onto the raw LEO imagery. 
    // TODO! Generalize.
    */
    cimg_library::CImg<unsigned char> drawMapToLEO(cimg_library::CImg<unsigned char> image, geodetic::projection::LEOScanProjector &projector);
};