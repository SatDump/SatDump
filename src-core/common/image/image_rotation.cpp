#include "image.h"

namespace image
{
    template <typename T>
    void Image<T>::rotate_left()
    {
        Image<T> tmp = *this;

        init(d_height, d_width, d_channels);
        
        for (int c = 0; c < d_channels; c++)
        {
            for (int x = 0; x < d_width; x++)
            {
                for (int y = 0; y < d_height; y++)
                {
                   channel(c)[x + (d_height-1-y)*d_width] = tmp.channel(c)[y + x*d_height];
                }
            }
        }
        
    }
    template <typename T>
    void Image<T>::rotate_right()
    {
        Image<T> tmp = *this;

        init(d_height, d_width, d_channels);
        
        for (int c = 0; c < d_channels; c++)
        {
            for (int x = 0; x < d_width; x++)
            {
                for (int y = 0; y < d_height; y++)
                {
                   channel(c)[d_width-x + y*d_width] = tmp.channel(c)[y + x*d_height];
                }
            }
        }
        
    }

    template class Image<uint8_t>;
    template class Image<uint16_t>;
}