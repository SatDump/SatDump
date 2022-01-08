#include "hue_saturation.h"
#include <limits>

namespace image
{
    template <typename T>
    T clamp(T value, T max, T min)
    {
        if (value > max)
            value = max;
        else if (value < min)
            value = min;
        return value;
    };

    HueSaturation::HueSaturation()
    {
        overlap = 0.0f;

        for (int i = HUE_RANGE_ALL; i <= HUE_RANGE_MAGENTA; i++)
        {
            HueRange partition = (HueRange)i;
            hue[partition] = 0.0;
            lightness[partition] = 0.0;
            saturation[partition] = 0.0;
        }
    }

    double map_hue(HueSaturation config, HueRange range, double value)
    {
        value += (config.hue[HUE_RANGE_ALL] + config.hue[range]) / 2.0;

        if (value < 0)
            return value + 1.0;
        else if (value > 1.0)
            return value - 1.0;
        else
            return value;
    }

    double map_hue_overlap(HueSaturation config, HueRange primary_range, HueRange secondary_range, double value, double primary_intensity, double secondary_intensity)
    {
        double v = config.hue[primary_range] * primary_intensity +
                   config.hue[secondary_range] * secondary_intensity;

        value += (config.hue[HUE_RANGE_ALL] + v) / 2.0;

        if (value < 0)
            return value + 1.0;
        else if (value > 1.0)
            return value - 1.0;
        else
            return value;
    }

    double map_saturation(HueSaturation config,
                          HueRange range,
                          double value)
    {
        double v = config.saturation[HUE_RANGE_ALL] + config.saturation[range];
        value *= (v + 1.0);
        return clamp<double>(value, 1.0, 0.0);
    }

    double map_lightness(HueSaturation config, HueRange range, double value)
    {
        double v = (config.lightness[HUE_RANGE_ALL] + config.lightness[range]);
        if (v < 0)
            return value * (v + 1.0);
        else
            return value + (v * (1.0 - value));
    }

    template <typename T>
    void hue_saturation(Image<T> &image, HueSaturation config)
    {
        float scale = std::numeric_limits<T>::max() - 1;

        double rgb_r, rgb_g, rgb_b;
        double hsl_h, hsl_s, hsl_l;

        float overlap = config.overlap / 2.0;

        for (int pixel = 0; pixel < image.width() * image.height(); pixel++)
        {
            rgb_r = image[image.width() * image.height() * 0 + pixel] / scale;
            rgb_g = image[image.width() * image.height() * 1 + pixel] / scale;
            rgb_b = image[image.width() * image.height() * 2 + pixel] / scale;

            rgb_to_hsl(rgb_r, rgb_g, rgb_b, hsl_h, hsl_s, hsl_l);
            {
                double h;
                int hue_counter;
                int hue = 0;
                int secondary_hue = 0;
                bool use_secondary_hue = false;
                float primary_intensity = 0.0;
                float secondary_intensity = 0.0;

                h = hsl_h * 6.0;

                for (hue_counter = 0; hue_counter < 7; hue_counter++)
                {
                    double hue_threshold = (double)hue_counter + 0.5;

                    if (h < ((double)hue_threshold + overlap))
                    {
                        hue = hue_counter;

                        if (overlap > 0.0 && h > ((double)hue_threshold - overlap))
                        {
                            use_secondary_hue = true;

                            secondary_hue = hue_counter + 1;

                            secondary_intensity = (h - (double)hue_threshold + overlap) / (2.0 * overlap);

                            primary_intensity = 1.0 - secondary_intensity;
                        }
                        else
                        {
                            use_secondary_hue = false;
                        }

                        break;
                    }
                }

                if (hue >= 6)
                {
                    hue = 0;
                    use_secondary_hue = false;
                }

                if (secondary_hue >= 6)
                {
                    secondary_hue = 0;
                }

                hue++;
                secondary_hue++;

                if (use_secondary_hue)
                {
                    hsl_h = map_hue_overlap(config, (HueRange)hue, (HueRange)secondary_hue, hsl_h, primary_intensity, secondary_intensity);
                    hsl_s = (map_saturation(config, (HueRange)hue, hsl_s) * primary_intensity + map_saturation(config, (HueRange)secondary_hue, hsl_s) * secondary_intensity);
                    hsl_l = (map_lightness(config, (HueRange)hue, hsl_l) * primary_intensity + map_lightness(config, (HueRange)secondary_hue, hsl_l) * secondary_intensity);
                }
                else
                {
                    hsl_h = map_hue(config, (HueRange)hue, hsl_h);
                    hsl_s = map_saturation(config, (HueRange)hue, hsl_s);
                    hsl_l = map_lightness(config, (HueRange)hue, hsl_l);
                }
            }
            hsl_to_rgb(hsl_h, hsl_s, hsl_l, rgb_r, rgb_g, rgb_b);

            image[image.width() * image.height() * 0 + pixel] = rgb_r * scale;
            image[image.width() * image.height() * 1 + pixel] = rgb_g * scale;
            image[image.width() * image.height() * 2 + pixel] = rgb_b * scale;
        }
    }

    void rgb_to_hsl(double r, double g, double b, double &h, double &s, double &l)
    {
        double max, min, delta;

        max = std::max<double>(r, std::max<double>(g, b));
        min = std::min<double>(r, std::min<double>(g, b));

        l = (max + min) / 2.0;

        if (max == min)
        {
            s = 0.0;
            h = -1.0;
        }
        else
        {
            if (l <= 0.5)
                s = (max - min) / (max + min);
            else
                s = (max - min) / (2.0 - max - min);

            delta = max - min;

            if (delta == 0.0)
                delta = 1.0;

            if (r == max)
            {
                h = (g - b) / delta;
            }
            else if (g == max)
            {
                h = 2.0 + (b - r) / delta;
            }
            else
            {
                h = 4.0 + (r - g) / delta;
            }

            h /= 6.0;

            if (h < 0.0)
                h += 1.0;
        }
    }

    double hsl_value(double n1, double n2, double hue)
    {
        double val;

        if (hue > 6.0)
            hue -= 6.0;
        else if (hue < 0.0)
            hue += 6.0;

        if (hue < 1.0)
            val = n1 + (n2 - n1) * hue;
        else if (hue < 3.0)
            val = n2;
        else if (hue < 4.0)
            val = n1 + (n2 - n1) * (4.0 - hue);
        else
            val = n1;

        return val;
    }

    void hsl_to_rgb(double h, double s, double l, double &r, double &g, double &b)
    {
        if (s == 0)
        {
            r = l;
            g = l;
            b = l;
        }
        else
        {
            double m1, m2;

            if (l <= 0.5)
                m2 = l * (1.0 + s);
            else
                m2 = l + s - l * s;

            m1 = 2.0 * l - m2;

            r = hsl_value(m1, m2, h * 6.0 + 2.0);
            g = hsl_value(m1, m2, h * 6.0);
            b = hsl_value(m1, m2, h * 6.0 - 2.0);
        }
    }

    template void hue_saturation<uint8_t>(Image<uint8_t> &, HueSaturation);
    template void hue_saturation<uint16_t>(Image<uint16_t> &, HueSaturation);
}