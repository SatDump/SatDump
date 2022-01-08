#pragma once

#include "image.h"

namespace image
{
    namespace bowtie
    {
        template <typename T>
        Image<T> correctGenericBowTie(Image<T> &inputImage, const int channelCount, const long scanHeight, const float alpha, const float beta);

        //template <typename T>
        //image::Image<T> correctSingleBowTie(image::Image<T> &inputImage, const int channelCount, const long scanHeight, const float alpha, const float beta);
    }
}