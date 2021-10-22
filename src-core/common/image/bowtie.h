#pragma once

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace image
{
    namespace bowtie
    {
        template <typename T>
        cimg_library::CImg<T> correctGenericBowTie(cimg_library::CImg<T> &inputImage, const int channelCount, const long scanHeight, const float alpha, const float beta);

        //template <typename T>
        //cimg_library::CImg<T> correctSingleBowTie(cimg_library::CImg<T> &inputImage, const int channelCount, const long scanHeight, const float alpha, const float beta);
    }
}