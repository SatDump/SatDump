#pragma once
#include "image.h"

namespace satdump
{
    namespace image
    { // TODOIMG maybe not a template?
        template <typename T>
        Image create_lut(int channels, int width, int points, std::vector<T> data);

        template <typename T>
        Image LUT_jet();

        inline Image scale_lut(int width, int x0, int x1, Image in)
        {
            in.resize(x1 - x0, 1);
            Image out(in.depth(), width, 1, 3);
            out.fill(0);
            out.draw_image(0, in, x0, 0);
            return out;
        }
    } // namespace image
} // namespace satdump