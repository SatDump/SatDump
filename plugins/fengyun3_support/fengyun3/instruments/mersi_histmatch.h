#pragma once

#include "common/image/image.h"

namespace fengyun3
{
    namespace mersi
    {
        void mersi_match_detector_histograms(image::Image<uint16_t> &img, int ndet);
    };
};