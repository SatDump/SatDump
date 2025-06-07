#pragma once

/**
 * @file image_utils.h
 */

#include "image.h"
#include <functional>

namespace satdump
{
    namespace image
    {
        /**
         * @brief  Simple function to make a "matrix" out of many image to save them in a
         * single image. You should ensure img_cnt is never 0 and does not exceed what
         * get_img_func can provide. Images should also be identical in size and depth!
         * @param count_width Matrix width in images
         * @param count_height Matrix height in images
         * @param img_cnt number of images
         * @param get_img_func function returns the images to put in the matrix
         * @return the image matrix
         */
        Image make_manyimg_composite(int count_width, int count_height, int img_cnt, std::function<Image(int cnt)> get_img_func);

        // TODOREWORK this might change?
        Image blend_images(std::vector<Image> &images);
        Image merge_images_opacity(Image &img1, Image &img2, float op);
    } // namespace image
} // namespace satdump