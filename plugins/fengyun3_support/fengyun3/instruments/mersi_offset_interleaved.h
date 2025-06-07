#pragma once

#include "image/image.h"

namespace fengyun3
{
    namespace mersi
    {
        void mersi_offset_interleaved(image::Image &img, int ndet, int shift);
    };
};