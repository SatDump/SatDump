#pragma once

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace image
{
    namespace bowtie
    {
        cimg_library::CImg<unsigned short> correctGenericBowTie(cimg_library::CImg<unsigned short> &inputImage, const int channelCount, const long scanHeight, const float alpha, const float beta);
    }
}