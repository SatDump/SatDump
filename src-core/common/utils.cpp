#include "utils.h"
#include <cmath>

void char_array_to_uchar(int8_t *in, uint8_t *out, int nsamples)
{
    for (int i = 0; i < nsamples; i++)
    {
        double r = in[i] * 127.0;
        if (r < 0)
            r = 0;
        else if (r > 255)
            r = 255;
        out[i] = floor(r);
    }
}