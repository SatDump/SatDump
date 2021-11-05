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
    void drawProjectedMapGeoJson(std::vector<std::string> shapeFiles, cimg_library::CImg<T> &image, T color[3], std::function<std::pair<int, int>(float, float, int, int)> projectionFunc);
    template <typename T>
    void drawProjectedCapitalsGeoJson(std::vector<std::string> shapeFiles, cimg_library::CImg<T> &image, T color[3], std::function<std::pair<int, int>(float, float, int, int)> projectionFunc, float ratio = 1);
    template <typename T>
    void drawProjectedMapShapefile(std::vector<std::string> shapeFiles, cimg_library::CImg<T> &image, T color[3], std::function<std::pair<int, int>(float, float, int, int)> projectionFunc);

    struct CustomLabel
    {
        std::string label;
        double lat;
        double lon;
    };

    template <typename T>
    void drawProjectedLabels(std::vector<CustomLabel> labels, cimg_library::CImg<T> &image, T color[3], std::function<std::pair<int, int>(float, float, int, int)> projectionFunc, float ratio = 1);
}