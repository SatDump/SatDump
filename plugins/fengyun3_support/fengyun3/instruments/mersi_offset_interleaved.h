#pragma once

#include "common/image2/image.h"

namespace fengyun3
{
    namespace mersi
    {
        void mersi_offset_interleaved(image2::Image &img, int ndet, int shift);
    };
};