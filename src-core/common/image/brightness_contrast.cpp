#include "brightness_contrast.h"

namespace image
{
    template <typename T>
    void brightness_contrast(cimg_library::CImg<T> &image, float brightness, float contrast, int channelCount)
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

    template void brightness_contrast<unsigned char>(cimg_library::CImg<unsigned char> &, float, float, int);
    template void brightness_contrast<unsigned short>(cimg_library::CImg<unsigned short> &, float, float, int);
}