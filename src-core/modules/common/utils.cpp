#include "utils.h"
#include <cmath>

void char_array_to_uchar(const char *in, unsigned char *out, int nsamples)
{
    for (int i = 0; i < nsamples; i++)
    {
        long int r = (long int)rint((float)in[i] * 127.0);
        if (r < 0)
            r = 0;
        else if (r > 255)
            r = 255;
        out[i] = r;
    }
}