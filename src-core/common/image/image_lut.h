#pragma once
#include "image.h"

namespace image
{ // TODOIMG maybe not a template?
    template <typename T>
    Image legacy_create_lut(int channels, int width, int points, std::vector<T> data);

    template <typename T>
    Image LUT_jet();

    inline Image legacy_scale_lut(int width, int x0, int x1, Image in)
    {
        in.resize(x1 - x0, 1);
        Image out(in.depth(), width, 1, 3);
        out.fill(0);
        out.draw_image(0, in, x0, 0);
        return out;
    }

    template <typename T>
    Image generate_lut(int width, std::vector<lut_point> p);
    std::vector<double> get_color_lut(double f, std::vector<lut_point> p);
}
