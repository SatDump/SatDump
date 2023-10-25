#include "image.h"
#include <limits>
#include <math.h>

#include <iostream>

namespace image
{
    template <typename T>
    Image<T> legacy_create_lut(int channels, int width, int points, std::vector<T> data)
    {
        Image<T> out(data.data(), points, 1, channels);
        out.resize_bilinear(width, 1, false);
        return out;
    }

    template <typename T>
    Image<T> scale_lut(int width, int x0, int x1, Image<T> in)
    {
        in.resize(x1 - x0, 1);
        image::Image<T> out(width, 1, 3);
        out.fill(0);
        out.draw_image(0, in, x0, 0);
        return out;
    }

    template <typename T>
    Image<T> LUT_jet()
    {
        return legacy_create_lut<T>(3,
                                    256,
                                    4,
                                    {0,
                                     0,
                                     std::numeric_limits<T>::max(),
                                     std::numeric_limits<T>::max(),
                                     0,
                                     std::numeric_limits<T>::max(),
                                     std::numeric_limits<T>::max(),
                                     0,
                                     std::numeric_limits<T>::max(),
                                     std::numeric_limits<T>::max(),
                                     0,
                                     0});
    }

    template Image<uint8_t> legacy_create_lut(int, int, int, std::vector<uint8_t>);
    template Image<uint16_t> legacy_create_lut(int, int, int, std::vector<uint16_t>);

    template Image<uint8_t> scale_lut(int, int, int, Image<uint8_t>);
    template Image<uint16_t> scale_lut(int, int, int, Image<uint16_t>);

    template Image<uint8_t> LUT_jet();
    template Image<uint16_t> LUT_jet();

    // END LEGACY

    float lin_to_sRGB(float x)
    {
        return x <= 0.0031308 ? 12.92 * x : (1.055 * pow(x, (1 / 2.4))) - 0.055;
    }

    float sRGB_to_lin(float x)
    {
        return x <= 0.04045 ? x / 12.92 : pow(((x + 0.055) / 1.055), 2.4);
    }

    float lerp(float c1, float c2, float f)
    {
        return c1 * (1.0 - f) + c2 * f;
    }

    std::vector<float> get_color_lut(float f, std::vector<lut_point> p)
    {
        if (f < p[0].f || f > p[p.size() - 1].f)
            return {0, 0, 0};

        int i = 0;
        for (i; f > p[i + 1].f; i++)
            ;
        if (p[i].f == p[i + 1].f)
            i++;

        if (i >= p.size() - 1)
        {
            i = p.size() - 2;
        }

        float gamma = 0.43;
        std::vector<float> c_out(3);
        std::vector<float> c_out2(3);
        float frac = (f - p[i].f) / (p[i + 1].f - p[i].f);
        float b1 = 0, b2 = 0;
        for (int c = 0; c < 3; c++)
        {
            float c1_lin = sRGB_to_lin(p[i].color[c]);
            b1 += c1_lin;
            float c2_lin = sRGB_to_lin(p[i + 1].color[c]);
            b2 += c2_lin;
            float color = lerp(c1_lin, c2_lin, frac);
            c_out[c] = color;
        }

        b1 = pow(b1, gamma);
        b2 = pow(b2, gamma);
        float intensity = pow(lerp(b1, b2, frac), 1 / gamma);

        if (c_out[0] + c_out[1] + c_out[2] != 0)
        {
            for (int c = 0; c < 3; c++)
            {
                c_out2[c] = c_out[c] * intensity / (c_out[0] + c_out[1] + c_out[2]);
            }
        }

        return {lin_to_sRGB(c_out2[0]), lin_to_sRGB(c_out2[1]), lin_to_sRGB(c_out2[2])};
    }

    template <typename T>
    Image<T> generate_lut(int width, std::vector<lut_point> p)
    {
        Image<T> out(width, 1, 3);
        for (int x = 0; x < width; x++)
        {
            std::vector<float> color = get_color_lut(float(x) / float(width - 1), p);
            T color_i[3] =
                {std::numeric_limits<T>::max() * color[0],
                 std::numeric_limits<T>::max() * color[1],
                 std::numeric_limits<T>::max() * color[2]};
            out.draw_pixel(x, 0, color_i);
        }
        return out;
    }

    template Image<uint8_t> generate_lut(int width, std::vector<lut_point> p);
    template Image<uint16_t> generate_lut(int width, std::vector<lut_point> p);
} // namespace image