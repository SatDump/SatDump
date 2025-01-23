#pragma once

#include <string>
#include "image.h"

namespace image
{
    struct EquationChannel
    {
        std::string tkt;
        image::Image *img;
        double val[4] = {0, 0, 0, 0};
    };

    image::Image generate_image_equation(std::vector<EquationChannel> channels, std::string equation);
}