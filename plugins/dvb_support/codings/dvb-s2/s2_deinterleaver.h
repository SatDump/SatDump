#pragma once

#include <cstdint>
#include "common/codings/dvb-s2/dvbs2.h"

namespace dvbs2
{
    class S2Deinterleaver
    {
    private:
        int mod_bits;
        int rows;
        int frame_length;

        int column0 = 0;
        int column1 = 0;
        int column2 = 0;
        int column3 = 0;
        int column4 = 0;

    public:
        S2Deinterleaver(dvbs2_constellation_t constellation, dvbs2_framesize_t framesize, dvbs2_code_rate_t rate);
        ~S2Deinterleaver();

        void deinterleave(int8_t *input, int8_t *output);
        void interleave(uint8_t *input, uint8_t *output);
    };
}