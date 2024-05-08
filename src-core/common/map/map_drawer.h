#pragma once

#include "common/image/image.h"
#include "common/image/text.h"
#include <functional>
#include <vector>
#include <string>
#include <limits>

namespace map
{
    void drawProjectedMapGeoJson(std::vector<std::string> shapeFiles,
                                 image::Image &image,
                                 std::vector<double> color,
                                 std::function<std::pair<int, int>(double, double, int, int)> projectionFunc,
                                 int maxLength = 2147483647);

    void drawProjectedCitiesGeoJson(std::vector<std::string> shapeFiles,
                                    image::Image &image,
                                    image::TextDrawer &text_drawer,
                                    std::vector<double> color,
                                    std::function<std::pair<int, int>(double, double, int, int)> projectionFunc,
                                    int font_size = 50,
                                    int cities_type = 0,
                                    int cities_scale_rank = 10);

    void drawProjectedMapShapefile(std::vector<std::string> shapeFiles,
                                   image::Image &image,
                                   std::vector<double> color,
                                   std::function<std::pair<int, int>(double, double, int, int)> projectionFunc);

    void drawProjectedMapLatLonGrid(image::Image &image,
                                    std::vector<double> color,
                                    std::function<std::pair<int, int>(double, double, int, int)> projectionFunc);

    /*  struct CustomLabel
      {
          std::string label;
          double lat;
          double lon;
      };

      void drawProjectedLabels(std::vector<CustomLabel> labels,
                               image::Image &image,
                               image::TextDrawer &text_drawer,
                               std::vector<double> color,
                               std::function<std::pair<int, int>(double, double, int, int)> projectionFunc,
                               double ratio = 1); */
}