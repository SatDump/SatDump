#include "image_utils.h"

namespace image
{
    image::Image<uint16_t> blend_images(image::Image<uint16_t> &img1, image::Image<uint16_t> &img2)
    {
        size_t width = std::min<int>(img1.width(), img2.width());
        size_t height = std::min<int>(img1.height(), img2.height());
        image::Image<uint16_t> img_b(width, height, img1.channels());
        bool are_rgba = img1.channels() == 4 && img2.channels() == 4;

        for (int c = 0; c < img1.channels(); c++)
        {
            if (are_rgba)
            {
                for (size_t i = 0; i < height * width; i++)
                {
                    if (img1.channel(3)[i] == 0)
                    {
                        img_b.channel(c)[i] = img2.channel(c)[i];
                        img_b.channel(3)[i] = 65535;
                    }
                    else if (img2.channel(3)[i] == 0)
                    {
                        img_b.channel(c)[i] = img1.channel(c)[i];
                        img_b.channel(3)[i] = 65535;
                    }
                    else
                    {
                        img_b.channel(c)[i] = c == 3 ? 65535 : ((size_t(img1.channel(c)[i]) + size_t(img2.channel(c)[i])) / 2);
                        img_b.channel(3)[i] = 65535;
                    }
                }
            }
            else
            {
                for (size_t i = 0; i < height * width; i++)
                {
                    if ((img1.channels() == 3 ? (uint64_t)img1.channel(0)[i] + (uint64_t)img1.channel(1)[i] + (uint64_t)img1.channel(2)[i] : img1.channel(c)[i]) == 0)
                        img_b.channel(c)[i] = img2.channel(c)[i];
                    else if ((img2.channels() == 3 ? (uint64_t)img2.channel(0)[i] + (uint64_t)img2.channel(1)[i] + (uint64_t)img2.channel(2)[i] : img2.channel(c)[i]) == 0)
                        img_b.channel(c)[i] = img1.channel(c)[i];
                    else
                        img_b.channel(c)[i] = (size_t(img1.channel(c)[i]) + size_t(img2.channel(c)[i])) / 2;
                }
            }
        }
        return img_b;
    }

    image::Image<uint16_t> merge_images_opacity(image::Image<uint16_t> &img1, image::Image<uint16_t> &img2, float op)
    {
        const size_t width = std::min<int>(img1.width(), img2.width());
        const size_t height = std::min<int>(img1.height(), img2.height());
        const int64_t size = width * height;
        const int channels_1 = img1.channels();
        const int channels_2 = img2.channels();
        const int color_channels = std::min(3, channels_1);
        image::Image<uint16_t> ret(width, height, channels_1);

#pragma omp parallel for
        for (int64_t i = 0; i < size; i++)
        {
            float alpha_1 = channels_1 == 4 ? (float)img1.channel(3)[i] / 65535.0f : 1.0f;
            float alpha_2 = (channels_2 == 4 ? (float)img2.channel(3)[i] / 65535.0f : 1.0f) * op;
            float ret_alpha = alpha_2 + alpha_1 * (1.0f - alpha_2);
            for (int j = 0; j < color_channels; j++)
                ret.channel(j)[i] = ((alpha_2 * ((float)img2.channel(j)[i] / 65535.0f) +
                    alpha_1 * ((float)img1.channel(j)[i] / 65535.0f) * (1.0f - alpha_2)) / ret_alpha) * 65535.0f;

            if (channels_1 == 4)
                ret.channel(3)[i] = ret_alpha * 65535.0f;
            else
                for (int j = 0; j < color_channels; j++)
                    ret.channel(j)[i] = (float)ret.channel(j)[i] * ret_alpha;
        }

        return ret;
    }
}