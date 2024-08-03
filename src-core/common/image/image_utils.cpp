#include "image_utils.h"
#include "core/exception.h"

namespace image
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

    Image blend_images(Image &img1, Image &img2)
    {
        if (img1.depth() != img2.depth() || img1.depth() != 16 || img2.depth() != 16)
            throw satdump_exception("blend_images must be the same bit depth, and 16");

        size_t width = std::min<int>(img1.width(), img2.width());
        size_t height = std::min<int>(img1.height(), img2.height());
        Image img_b(img1.depth(), width, height, img1.channels());
        bool are_rgba = img1.channels() == 4 && img2.channels() == 4;

        for (int c = 0; c < img1.channels(); c++)
        {
            if (are_rgba)
            {
                for (size_t i = 0; i < height * width; i++)
                {
                    float ch1_alpha = img1.getf(3, i);
                    float ch2_alpha = img2.getf(3, i);
                    if (c == 3)
                        img_b.setf(c, i, std::max(ch1_alpha, ch2_alpha));
                    else if (ch1_alpha == 0)
                        img_b.set(c, i, img2.get(c, i));
                    else if (ch2_alpha == 0)
                        img_b.set(c, i, img1.get(c, i));
                    else
                        img_b.setf(c, i, (img1.getf(c, i) * ch1_alpha + img2.getf(c, i) * ch2_alpha) / 2);
                }
            }
            else
            {
                for (size_t i = 0; i < height * width; i++)
                {
                    if ((img1.channels() == 3 ? (uint64_t)img1.get(0, i) + (uint64_t)img1.get(1, i) + (uint64_t)img1.get(2, i) : img1.get(c, i)) == 0)
                        img_b.set(c, i, img2.get(c, i));
                    else if ((img2.channels() == 3 ? (uint64_t)img2.get(0, i) + (uint64_t)img2.get(1, i) + (uint64_t)img2.get(2, i) : img2.get(c, i)) == 0)
                        img_b.set(c, i, img1.get(c, i));
                    else
                        img_b.set(c, i, (size_t(img1.get(c, i)) + size_t(img2.get(c, i))) / 2);
                }
            }
        }
        return img_b;
    }

    image::Image merge_images_opacity(image::Image &img1, image::Image &img2, float op)
    {
        if (img1.depth() != img2.depth() || img1.depth() != 16 || img2.depth() != 16)
            throw satdump_exception("merge_images_opacity must be the same bit depth, and 16");

        const size_t width = std::min<int>(img1.width(), img2.width());
        const size_t height = std::min<int>(img1.height(), img2.height());
        const int64_t size = width * height;
        const int channels_1 = img1.channels();
        const int channels_2 = img2.channels();
        const int color_channels = std::min(3, channels_1);
        image::Image ret(16, width, height, channels_1);

#pragma omp parallel for
        for (int64_t i = 0; i < size; i++)
        {
            float alpha_1 = channels_1 == 4 ? (float)img1.get(3, i) / 65535.0f : 1.0f;
            float alpha_2 = (channels_2 == 4 ? (float)img2.get(3, i) / 65535.0f : 1.0f) * op;
            float ret_alpha = alpha_2 + alpha_1 * (1.0f - alpha_2);
            for (int j = 0; j < color_channels; j++)
                ret.set(j, i, ((alpha_2 * ((float)img2.get(j, i) / 65535.0f) + alpha_1 * ((float)img1.get(j, i) / 65535.0f) * (1.0f - alpha_2)) / ret_alpha) * 65535.0f);

            if (channels_1 == 4)
                ret.set(3, i, ret_alpha * 65535.0f);
            else
                for (int j = 0; j < color_channels; j++)
                    ret.set(j, i, (float)ret.get(j, i) * ret_alpha);
        }

        return ret;
    }
}