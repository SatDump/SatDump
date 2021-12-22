#pragma once

#include "common/image/image.h"
#include <functional>
#include <vector>
#include <string>
#include <limits>

namespace map
{
    template <typename T>
    void drawProjectedMapGeoJson(std::vector<std::string> shapeFiles, image::Image<T> &image, T color[3], std::function<std::pair<int, int>(float, float, int, int)> projectionFunc, int maxLength = std::numeric_limits<int>::max());
    template <typename T>
    void drawProjectedCapitalsGeoJson(std::vector<std::string> shapeFiles, image::Image<T> &image, T color[3], std::function<std::pair<int, int>(float, float, int, int)> projectionFunc, float ratio = 1);
    template <typename T>
    void drawProjectedMapShapefile(std::vector<std::string> shapeFiles, image::Image<T> &image, T color[3], std::function<std::pair<int, int>(float, float, int, int)> projectionFunc, int maxLength = std::numeric_limits<int>::max());

    struct CustomLabel
    {
        std::string label;
        double lat;
        double lon;
    };

    template <typename T>
    void drawProjectedLabels(std::vector<CustomLabel> labels, image::Image<T> &image, T color[3], std::function<std::pair<int, int>(float, float, int, int)> projectionFunc, float ratio = 1);
}