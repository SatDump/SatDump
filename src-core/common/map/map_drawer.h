#pragma once

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include <functional>
#include <vector>
#include <string>

namespace map
{
    template <typename T>
    void drawProjectedMap(cimg_library::CImg<T> &image, std::vector<std::string> shapeFiles, T color[3], std::function<std::pair<int, int>(float, float, int, int)> projectionFunc);
}