#pragma once

#include <cstdint>
#include "image.h"

namespace image
{
    // Implementation of GIMP's White Hue Saturation algorithm
    // See app/base/hue-saturation.h
    // and app/base/hue-saturation.c
    // This was ported over from those 2 files
    // All credits go to the Gimp project
    // https://github.com/GNOME/gimp

    enum HueRange
    {
        HUE_RANGE_ALL,
        HUE_RANGE_RED,
        HUE_RANGE_YELLOW,
        HUE_RANGE_GREEN,
        HUE_RANGE_CYAN,
        HUE_RANGE_BLUE,
        HUE_RANGE_MAGENTA
    };

    struct HueSaturation
    {
        HueRange range;

        double hue[7];
        double saturation[7];
        double lightness[7];

        double overlap;

        HueSaturation();
    };

    // Apply hue saturation settings
    template <typename T>
    void hue_saturation(Image<T> &image, HueSaturation hueSaturation);

    void rgb_to_hsl(double r, double g, double b, double &h, double &s, double &l);
    void hsl_to_rgb(double h, double s, double l, double &r, double &g, double &b);
}