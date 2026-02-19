#pragma once

#include "image/image.h"

namespace kanopus
{
    namespace pmss
    {
        struct PMSSSegment
        {
            image::Image img;
            int ccd_id;

            PMSSSegment(image::Image img, int ccd_id) : img(img), ccd_id(ccd_id) {}
        };
    } // namespace pmss
} // namespace kanopus