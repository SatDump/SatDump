#pragma once

#include <cstdint>
#include <vector>
#include <cstring>

extern "C"
{
#include "mbelib/mbelib.h"
}

namespace inmarsat
{
    namespace aero
    {
        void deinterleave(int8_t *input, int8_t *output, int cols);
        int depuncture(int8_t *in, uint8_t *out, int shift, int size);
        void unpack_areo_c84_packet(uint8_t *input, uint8_t *voice, uint8_t *blocks);

        // Simple, slightly modified port of
        // https://github.com/jontio/libaeroambe
        class AmbeDecoder
        {
        private:
            mbe_parms current;
            mbe_parms prev;
            mbe_parms prev_mp_enhanced;

            const unsigned char rX[96] = {
                23, 11, 14, 2, 5, 8, 9, 11, 22, 10, 13, 1, 4, 7, 8, 10, 21, 9, 12, 0, 3, 6, 7, 9, 20, 8, 11, 14, 2, 5, 6,
                8, 19, 7, 10, 13, 1, 4, 5, 7, 18, 6, 9, 12, 0, 3, 4, 6, 17, 5, 8, 11, 14, 2, 3, 5, 16, 4, 7, 10, 13, 1, 2,
                4, 15, 3, 6, 9, 12, 0, 1, 3, 14, 2, 5, 8, 11, 12, 0, 2, 13, 1, 4, 7, 10, 11, 13, 1, 12, 0, 3, 6, 9, 10, 12, 0};
            const unsigned char rW[96] = {
                0, 0, 1, 1, 2, 3, 4, 5, 0, 0, 1, 1, 2, 3, 4, 5, 0, 0, 1, 1, 2, 3, 4, 5, 0, 0, 1, 2, 2, 3, 4, 5, 0, 0, 1, 2,
                2, 3, 4, 5, 0, 0, 1, 2, 2, 3, 4, 5, 0, 0, 1, 2, 3, 3, 4, 5, 0, 0, 1, 2, 3, 3, 4, 5, 0, 0, 1, 2, 3, 3, 4, 5,
                0, 0, 1, 2, 3, 4, 4, 5, 0, 0, 1, 2, 3, 4, 5, 5, 0, 0, 1, 2, 3, 4, 5, 5};

            uint8_t ambe_bits[96];
            char ambe_fr[6][24];
            char ambe_d[72];

            char errorbuffer[1024];
            int errors = 0;
            int errors2 = 0;

        public:
            AmbeDecoder();
            void decode(uint8_t *data, int blocks, int16_t *out_audio);
        };
    }
}