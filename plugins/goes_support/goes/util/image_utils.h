#pragma once
#include "common/image/image.h"

namespace goes
{
    template <typename T>
    void fill_background(image::Image<T> *image, T bgcolor[], T replaceColor[])
    {
        bool stop;
        for (size_t i = 0; i < image->width() * image->width(); i += image->width())
        {
            int c;
            size_t j;

            //Left Side
            stop = false;
            for (j = 0; j < image->width(); j++)
            {
                for (c = 0; c < image->channels(); c++)
                {
                    if (image->channel(c)[i + j] != bgcolor[c])
                        stop = true;
                }
                if (stop) break;
                for (c = 0; c < image->channels(); c++)
                    image->channel(c)[i + j] = replaceColor[c];
            }

            //Right Side
            stop = false;
            for (j = image->width() - 1; j >= 0; j--)
            {
                for (c = 0; c < image->channels(); c++)
                {
                    if (image->channel(c)[i + j] != bgcolor[c])
                        stop = true;
                }
                if (stop) break;
                for (c = 0; c < image->channels(); c++)
                    image->channel(c)[i + j] = replaceColor[c];
            }
        }
    }
}