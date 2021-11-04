#pragma once

#include <cstdint>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include <string>

namespace image
{
    // Generate a composite from channels and an equation
    template <typename T>
    cimg_library::CImg<T> generate_composite_from_equ(std::vector<cimg_library::CImg<T>> inputChannels, std::vector<int> channelNumbers, std::string equation);
}