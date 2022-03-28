#pragma once

#include <cstdint>
#include <cmath>
#include "common/dsp/complex.h"

/*
Stuff from LeanDVB, which I could for sure have re-written...
But why? Already did what I needed :-)
*/
namespace dvbs2
{
    static const int cstln_amp = 1; // 75;

    struct s2_sof
    {
        static const uint32_t VALUE = 0x18d2e82;
        static const uint32_t MASK = 0x3ffffff;
        static const int LENGTH = 26;

        complex_t symbols[LENGTH];

        s2_sof()
        {
            for (int s = 0; s < LENGTH; ++s)
            {
                bool bit = ((VALUE >> (LENGTH - 1 - s)) & 1);
                int angle = bit * 2 + (s & 1); // pi/2-BPSK
                symbols[s].real = cstln_amp * cosf(M_PI / 4 + 2 * M_PI * angle / 4);
                symbols[s].imag = cstln_amp * sinf(M_PI / 4 + 2 * M_PI * angle / 4);
            }
        }
    };

    struct s2_plscodes
    {
        // PLS index format MODCOD[4:0]|SHORTFRAME|PILOTS
        static const int COUNT = 128;
        static const int LENGTH = 64;

        uint64_t codewords[COUNT];
        complex_t symbols[COUNT][LENGTH];

        s2_plscodes()
        {
            uint32_t G[6] = {0x55555555,
                             0x33333333,
                             0x0f0f0f0f,
                             0x00ff00ff,
                             0x0000ffff,
                             0xffffffff};

            for (int index = 0; index < COUNT; ++index)
            {
                uint32_t y = 0;
                for (int row = 0; row < 6; ++row)
                    if ((index >> (6 - row)) & 1)
                        y ^= G[row];

                uint64_t code = 0;
                for (int bit = 31; bit >= 0; --bit)
                {
                    int yi = (y >> bit) & 1;
                    if (index & 1)
                        code = (code << 2) | (yi << 1) | (yi ^ 1);
                    else
                        code = (code << 2) | (yi << 1) | yi;
                }

                // Scrambling
                code ^= SCRAMBLING;

                // Store precomputed codeword.
                codewords[index] = code;

                // Also store as symbols.
                for (int i = 0; i < LENGTH; ++i)
                {
                    int yi = (code >> (LENGTH - 1 - i)) & 1;
                    int nyi = yi ^ (i & 1);
                    symbols[index][i].real = cstln_amp * (1 - 2 * nyi) / sqrtf(2);
                    symbols[index][i].imag = cstln_amp * (1 - 2 * yi) / sqrtf(2);
                }
            }
        }

        static const uint64_t SCRAMBLING = 0x719d83c953422dfa;
    }; // s2_plscodes
}