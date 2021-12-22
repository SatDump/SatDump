#pragma once

#include "common/image/image.h"

namespace fengyun3
{
    namespace mersi
    {
        void banding_correct(image::Image<uint16_t> &imageo, int rowSize, float percent = 0.10);
    };
};