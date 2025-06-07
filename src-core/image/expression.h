#pragma once

#include "image.h"
#include <string>

namespace satdump
{
    namespace image
    {
        struct ExpressionChannel
        {
            std::string tkt;
            image::Image *img;
            double val[4] = {0, 0, 0, 0};
        };

        image::Image generate_image_expression(std::vector<ExpressionChannel> channels, std::string expression);
    } // namespace image
} // namespace satdump