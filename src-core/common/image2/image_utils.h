#pragma once

#include "image.h"
#include <functional>

namespace image2
{
    /*
    Simple function to make a "matrix" out of many image to save them
    in a single image.

    You should ensure img_cnt is never 0 and does not exceed what
    get_img_func can provide.
    */
    Image make_manyimg_composite(int count_width, int count_height, int img_cnt, std::function<Image(int cnt)> get_img_func);

    Image blend_images(Image &img1, Image &img2);
    Image merge_images_opacity(Image &img1, Image &img2, float op);
}