#pragma once

#include "common/image/image.h"

namespace fengyun3
{
    namespace mersi
    {
        void mersi_offset_interleaved(image::Image<uint16_t> &img, int ndet, int shift);
    };
};