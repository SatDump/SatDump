#pragma once

/**
 * @file hue_saturation.h
 * @brief Hue/Saturation manipulation
 */

#include "image.h"

namespace satdump
{
    namespace image
    {
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

        /**
         * @brief Hue/Saturation settings. Each
         * array should be access using HueRange
         * enum values.
         */
        struct HueSaturation
        {
            double hue[7];
            double saturation[7];
            double lightness[7];

            double overlap;

            HueSaturation();
        };

        /**
         * @brief Apply hue saturation settings.
         * Implementation of GIMP's White Hue Saturation algorithm
         * See app/base/hue-saturation.h
         * and app/base/hue-saturation.c
         * This was ported over from those 2 files
         * All credits go to the Gimp project
         * https://github.com/GNOME/gimp
         * @param image image to hue shift
         * @param hueSaturation parameters to use
         */
        void hue_saturation(Image &image, HueSaturation hueSaturation);

        /**
         * @brief Convert RGB to HSL
         */
        void rgb_to_hsl(double r, double g, double b, double &h, double &s, double &l);

        /**
         * @brief Convert HSL to RGB
         */
        void hsl_to_rgb(double h, double s, double l, double &r, double &g, double &b);
    } // namespace image
} // namespace satdump