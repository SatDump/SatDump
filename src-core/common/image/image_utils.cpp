#include "image_utils.h"
#include "core/exception.h"
#include <algorithm>

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

    Image blend_images(std::vector<Image> &images)
    {
        if(images.size() == 0)
            throw satdump_exception("No images passed!");

        size_t width = images[0].width();
        size_t height = images[0].height();
        int channels = images[0].channels();
        bool are_rgba = channels == 4;

        for (Image &this_img : images)
        {
            if(this_img.depth() != 16)
                throw satdump_exception("blend_images must be the same bit depth, and 16");
            width = std::min(width, this_img.width());
            height = std::min(height, this_img.height());
            are_rgba &= this_img.channels() == 4;
        }

        Image img_b(16, width, height, channels);

        for (int c = 0; c < channels; c++)
        {
            if (are_rgba)
            {
                for (size_t i = 0; i < height * width; i++)
                {
                    if (c == 3)
                    {
                        double final_alpha = 0.0;
                        for (Image &this_img : images)
                            final_alpha = std::max(final_alpha, this_img.getf(3, i));
                        img_b.setf(c, i, final_alpha);
                    }
                    else
                    {
                        float num_layers = 0.0f;
                        double final_val = 0.0;
                        for (Image &this_img : images)
                        {
                            final_val += this_img.getf(c, i) * this_img.getf(3, i);
                            num_layers += this_img.getf(3, i);
                        }

                        img_b.setf(c, i, final_val / num_layers);
                    }
                }
            }
            else
            {
                for (size_t i = 0; i < height * width; i++)
                {
                    size_t num_layers = images.size();
                    double final_val = 0.0f;
                    for (Image& this_img : images)
                    {
                        float layer_val = this_img.getf(c, i);
                        if (layer_val == 0)
                            num_layers--;
                        else
                            final_val += layer_val;
                    }

                    img_b.setf(c, i, final_val / num_layers);
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