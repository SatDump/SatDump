#include "image_lut.h"

namespace image
{
    template <typename T>
    Image create_lut(int channels, int width, int points, std::vector<T> data)
    {
        Image out(data.data(), sizeof(T) * 8, points, 1, channels);
        out.resize_bilinear(width, 1, false);
        return out;
    }

    template <typename T>
    Image LUT_jet()
    {
        return create_lut<T>(3,
            256,
            4,
            { 0,
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
             0 });
    }

    template Image create_lut<uint8_t>(int, int, int, std::vector<uint8_t>);
    template Image create_lut<uint16_t>(int, int, int, std::vector<uint16_t>);
    template Image LUT_jet<uint8_t>();
    template Image LUT_jet<uint16_t>();
}