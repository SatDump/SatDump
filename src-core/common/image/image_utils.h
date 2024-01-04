#pragma once

#include "image.h"

namespace image
{
    image::Image<uint16_t> blend_images(image::Image<uint16_t> &img1, image::Image<uint16_t> &img2);
    image::Image<uint16_t> merge_images_opacity(image::Image<uint16_t> &img1, image::Image<uint16_t> &img2, float op);
}