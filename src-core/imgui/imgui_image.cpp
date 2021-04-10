#define SATDUMP_DLL_EXPORT 1
#include "imgui_image.h"

SATDUMP_DLL std::function<unsigned int()> makeImageTexture;
SATDUMP_DLL std::function<void(unsigned int, uint32_t *, int, int)> updateImageTexture;
SATDUMP_DLL std::function<void(unsigned int)> deleteImageTexture;

void ushort_to_rgba(uint16_t *input, uint32_t *output, int size, int channels)
{
    if (channels == 1)
    {
        for (int i = 0; i < size; i++)
        {
            uint8_t c;
            c = input[i] >> 8;
            output[i] = ((uint32_t)255 << 24) | ((uint32_t)c << 16) | ((uint32_t)c << 8) | (uint32_t)c;
        }
    }
    else if (channels == 3)
    {
        for (int i = 0; i < size; i++)
        {
            uint8_t r, g, b;
            r = input[size * 0 + i] >> 8;
            g = input[size * 1 + i] >> 8;
            b = input[size * 2 + i] >> 8;
            output[i] = ((uint32_t)255 << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
        }
    }
}