#pragma once

#include <string>
#include "image.h"

namespace image
{
    struct ExpressionChannel
    {
        std::string tkt;
        image::Image *img;
        double val[4] = {0, 0, 0, 0};
    };

    image::Image generate_image_expression(std::vector<ExpressionChannel> channels, std::string expression);
}