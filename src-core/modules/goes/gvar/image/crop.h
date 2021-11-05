#pragma once

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace goes
{
    namespace gvar
    {
        cimg_library::CImg<unsigned short> cropIR(cimg_library::CImg<unsigned short> input);
        cimg_library::CImg<unsigned short> cropVIS(cimg_library::CImg<unsigned short> input);
    };
};