#include "image.h"

namespace image2
{
    template <typename T>
    Image create_lut(int channels, int width, int points, std::vector<T> data)
    {
        Image out(data.data(), sizeof(T) * 8, points, 1, channels);
        out.resize_bilinear(width, 1, false);
        return out;
    }

    inline Image scale_lut(int width, int x0, int x1, Image in)
    {
        in.resize(x1 - x0, 1);
        Image out(in.depth(), width, 1, 3);
        out.fill(0);
        out.draw_image(0, in, x0, 0);
        return out;
    }

    template <typename T>
    Image LUT_jet()
    {
        return create_lut<T>(3,
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
}