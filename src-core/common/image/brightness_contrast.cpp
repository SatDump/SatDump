#include "brightness_contrast.h"
#include <cmath>

namespace image
{
    template <typename T>
    void brightness_contrast(Image<T> &image, float brightness, float contrast, int channelCount)
    {
        float scale = std::numeric_limits<T>::max() - 1;

        float brightness_v = brightness / 2.0f;
        float slant = tanf((contrast + 1.0f) * 0.78539816339744830961566084581987572104929234984378f);

        for (int i = 0; i < image.height() * image.width() * channelCount; i++)
        {
            float v = float(image[i]) / scale;

            if (brightness_v < 0.0)
                v = v * (1.0 + brightness_v);
            else
                v = v + ((1.0 - v) * brightness_v);

            v = (v - 0.5) * slant + 0.5;

            image[i] = std::min<float>(scale, std::max<float>(0, v * scale));
        }
    }

    template void brightness_contrast<uint8_t>(Image<uint8_t> &, float, float, int);
    template void brightness_contrast<uint16_t>(Image<uint16_t> &, float, float, int);
}