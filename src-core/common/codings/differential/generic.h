/*
    Diff decoder ported from GNU Radio
*/

#pragma once

#include <volk/volk_alloc.hh>

namespace diff
{
    class GenericDiff
    {
    private:
        const unsigned int d_modulus;
        volk::vector<uint8_t> tmp_;

    public:
        GenericDiff(unsigned int modulus);
        int work(uint8_t *in, int length, uint8_t *out);
    };
}