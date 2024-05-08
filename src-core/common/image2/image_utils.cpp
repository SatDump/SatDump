#include "image_utils.h"

#include <cstdio>

namespace image2
{
    Image make_manyimg_composite(int count_width, int count_height, int img_cnt, std::function<Image(int cnt)> get_img_func)
    {
        Image first_img = get_img_func(0);
        int img_w = first_img.width();
        int img_h = first_img.height();

        Image image_all(first_img.depth(), img_w * count_width, img_h * count_height, first_img.channels());

        first_img.clear();

        for (int row = 0; row < count_height; row++)
        {
            for (int column = 0; column < count_width; column++)
            {
                if (row * count_width + column >= img_cnt)
                    return image_all;

                image_all.draw_image(0, get_img_func(row * count_width + column), img_w * column, img_h * row);
            }
        }

        return image_all;
    }
}