#include "utils.h"

void signed_soft_to_unsigned(int8_t *in, uint8_t *out, int nsamples)
{
    for (int i = 0; i < nsamples; i++)
    {
        out[i] = in[i] + 127;

        if (out[i] == 128) // 128 is for erased syms
            out[i] = 127;
    }
}