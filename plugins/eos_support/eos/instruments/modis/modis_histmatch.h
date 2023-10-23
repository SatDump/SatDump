#pragma once

#include "common/image/image.h"

namespace eos
{
    namespace modis
    {
        void modis_match_detector_histograms(image::Image<uint16_t> &img, int ndetx, int ndety);
    };
};