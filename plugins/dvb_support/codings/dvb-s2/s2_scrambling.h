#pragma once

#include <cstdint>
#include "common/dsp/complex.h"
#include "common/codings/dvb-s2/dvbs2.h"

/*
Adapted from LeanDVB
*/

namespace dvbs2
{
    class S2Scrambling
    {
    private:
        uint8_t Rn[131072];

        uint32_t lfsr_x(uint32_t X)
        {
            int bit = ((X >> 7) ^ X) & 1;
            return ((bit << 18) | X) >> 1;
        }

        uint32_t lfsr_y(uint32_t Y)
        {
            int bit = ((Y >> 10) ^ (Y >> 7) ^ (Y >> 5) ^ Y) & 1;
            return ((bit << 18) | Y) >> 1;
        }

        complex_t res = 0;
        int r = 0;

        int pos = 0;

    public:
        S2Scrambling(int codenum = 0);
        ~S2Scrambling();

        void reset();
        complex_t descramble(complex_t &sample);
        complex_t scramble(complex_t &sample);
    };
}