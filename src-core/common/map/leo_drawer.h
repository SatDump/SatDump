#pragma once

#include "common/image/image.h"
#include <functional>
#include <vector>
#include "common/geodetic/projection/leo_projection.h"

namespace map
{
    /*
    Helper function to draw a map overlay onto the raw LEO imagery. 
    // TODO! Generalize.
    */
    image::Image<uint8_t> drawMapToLEO(image::Image<uint8_t> image, geodetic::projection::LEOScanProjector &projector);
};