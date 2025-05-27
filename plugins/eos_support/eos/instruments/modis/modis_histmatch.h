#pragma once

#include "image/image.h"

namespace eos
{
    namespace modis
    {
        void modis_match_detector_histograms(image::Image &img, int ndetx, int ndety);
    };
};