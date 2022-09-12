#pragma once

#include "common/image/image.h"

namespace satdump
{
    namespace reproj
    {
        void reproject_equ_to_stereo(
            image::Image<uint16_t> &source_img, // Source image
            float equ_tl_lon, float equ_tl_lat, // Top-Left corner
            float equ_br_lon, float equ_br_lat, // Bottom-Right corner
            image::Image<uint16_t> &target_img, // Target image
            float ste_center_lat,               // Stereo center lat
            float ste_center_lon,               // Stereo center lon
            float ste_scale,                    // Stereo Scale
            float *progress);
    }
}