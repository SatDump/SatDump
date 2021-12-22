#pragma once

#include "image.h"

namespace image
{
    namespace vegetation_index
    {
        Image<uint16_t> NDVI(Image<uint16_t> redIm, Image<uint16_t> nirIm);
        Image<uint16_t> EVI2(Image<uint16_t> redIm, Image<uint16_t> nirIm);
        Image<uint16_t> EVI(Image<uint16_t> redIm, Image<uint16_t> nirIm, Image<uint16_t> blueIm);
    }
}