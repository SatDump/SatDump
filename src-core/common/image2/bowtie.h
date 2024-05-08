#pragma once

#include "image.h"
#include <vector>

namespace image2
{
    namespace bowtie
    {
        Image correctGenericBowTie(Image &inputImage, const int channelCount, const long scanHeight, const float alpha, const float beta, std::vector<std::vector<int>> *reverse_lut = nullptr);

        // template <typename T>
        // image::Image<T> correctSingleBowTie(image::Image<T> &inputImage, const int channelCount, const long scanHeight, const float alpha, const float beta);
    }
}