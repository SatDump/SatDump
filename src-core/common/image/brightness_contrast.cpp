#include "brightness_contrast.h"
#include <cmath>

namespace image
{
    void brightness_contrast(Image &image, float brightness, float contrast)
    {
        int channelCount = image.channels();
        const float scale = image.maxval() - 1;

        float brightness_v = brightness / 2.0f;
        float slant = tanf((contrast + 1.0f) * 0.78539816339744830961566084581987572104929234984378f);

        if (channelCount == 4)
            channelCount = 3;

        for (size_t i = 0; i < image.height() * image.width() * channelCount; i++)
        {
            float v = float(image.get(i)) / scale;

            if (brightness_v < 0.0)
                v = v * (1.0 + brightness_v);
            else
                v = v + ((1.0 - v) * brightness_v);

            v = (v - 0.5) * slant + 0.5;

            image.set(i, std::min<float>(scale, std::max<float>(0, v * scale)));
        }
    }
}