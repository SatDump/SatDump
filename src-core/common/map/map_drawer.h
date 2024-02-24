#pragma once

#include "common/image/image.h"
#include <functional>
#include <vector>
#include <string>
#include <limits>

namespace map
{
    template <typename T>
    void drawProjectedMapGeoJson(std::vector<std::string> shapeFiles, image::Image<T> &image, T color[], std::function<std::pair<int, int>(double, double, int, int)> projectionFunc, int maxLength = 2147483647);
    template <typename T>
    void drawProjectedCitiesGeoJson(std::vector<std::string> shapeFiles, image::Image<T> &image, T color[], std::function<std::pair<int, int>(double, double, int, int)> projectionFunc, int font_size = 50, int cities_type = 0, int cities_scale_rank = 10);
    template <typename T>
    void drawProjectedMapShapefile(std::vector<std::string> shapeFiles, image::Image<T> &image, T color[], std::function<std::pair<int, int>(double, double, int, int)> projectionFunc);
    template <typename T>
    void drawProjectedMapLatLonGrid(image::Image<T> &image, T color[], std::function<std::pair<int, int>(double, double, int, int)> projectionFunc);

    struct CustomLabel
    {
        std::string label;
        double lat;
        double lon;
    };

    template <typename T>
    void drawProjectedLabels(std::vector<CustomLabel> labels, image::Image<T> &image, T color[], std::function<std::pair<int, int>(double, double, int, int)> projectionFunc, double ratio = 1);
}