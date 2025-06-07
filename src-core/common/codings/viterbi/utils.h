#pragma once

#include <cstdint>

void signed_soft_to_unsigned(int8_t *in, uint8_t *out, int nsamples);

inline int ensureIs2Multiple(int v)
{
    if (v % 2 == 1)
        v++;
    return v;
}