#pragma once

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace image{
    namespace vegetation_index{
        cimg_library::CImg<unsigned short> NDVI(cimg_library::CImg<unsigned short> redIm, cimg_library::CImg<unsigned short> nirIm);
        cimg_library::CImg<unsigned short> EVI2(cimg_library::CImg<unsigned short> redIm, cimg_library::CImg<unsigned short> nirIm);
        cimg_library::CImg<unsigned short> EVI(cimg_library::CImg<unsigned short> redIm, cimg_library::CImg<unsigned short> nirIm, cimg_library::CImg<unsigned short> blueIm);
    }
}