#include "image.h"
#include <limits>

namespace image
{
    template <typename T>
    Image<T> create_lut(int channels, int width, int points, std::vector<T> data)
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

    template Image<uint8_t> create_lut(int, int, int, std::vector<uint8_t>);
    template Image<uint16_t> create_lut(int, int, int, std::vector<uint16_t>);

    template Image<uint8_t> scale_lut(int, int, int, Image<uint8_t>);
    template Image<uint16_t> scale_lut(int, int, int, Image<uint16_t>);

    template Image<uint8_t> LUT_jet();
    template Image<uint16_t> LUT_jet();
} // namespace image