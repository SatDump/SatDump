#include "color.h"
#include <cmath>

namespace satdump
{
    void hsv_to_rgb(float h, float s, float v, uint8_t *rgb)
    {
        float out_r, out_g, out_b;

        if (s == 0.0f)
        {
            // gray
            out_r = out_g = out_b = v;

            rgb[0] = out_r * 255;
            rgb[1] = out_g * 255;
            rgb[2] = out_b * 255;

            return;
        }

        h = fmod(h, 1.0f) / (60.0f / 360.0f);
        int i = (int)h;
        float f = h - (float)i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        switch (i)
        {
        case 0:
            out_r = v;
            out_g = t;
            out_b = p;
            break;
        case 1:
            out_r = q;
            out_g = v;
            out_b = p;
            break;
        case 2:
            out_r = p;
            out_g = v;
            out_b = t;
            break;
        case 3:
            out_r = p;
            out_g = q;
            out_b = v;
            break;
        case 4:
            out_r = t;
            out_g = p;
            out_b = v;
            break;
        case 5:
        default:
            out_r = v;
            out_g = p;
            out_b = q;
            break;
        }

        rgb[0] = out_r * 255;
        rgb[1] = out_g * 255;
        rgb[2] = out_b * 255;
    }
} // namespace satdump