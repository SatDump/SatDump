#include "decode_utils.h"
#include <cstring>

#include <iostream>

namespace inmarsat
{
    namespace aero
    {
        // Syncword. Copied from Scytale-C as I was unable to find it somewhere else
        const uint8_t syncword[] = {1, 1, 1, 0, 0, 0, 0, 1,
                                    0, 1, 0, 1, 1, 0, 1, 0,
                                    1, 1, 1, 0, 1, 0, 0, 0,
                                    1, 0, 0, 1, 0, 0, 1, 1};

        int compute_frame_match(int8_t *syms, bool &inverted)
        {
            int match_nrm = 0;
            int match_inv = 0;

            for (int i = 0; i < 32; i++)
            {
                if ((syncword[i] ^ (syms[i] > 0)) == 0)
                    match_nrm++;
            }

            //  printf("IDK %d\n", match_nrm);

            inverted = false;

            return match_nrm;
        }

        void deinterleave(int8_t *input, int8_t *output, int cols)
        {
            const int rows = 64;
            int k = 0;
            for (int j = 0; j < cols; j++)
                for (int i = 0; i < rows; i++)
                    output[k++] = input[((i * 27) % rows) * cols + j];
        }

        /*
                void descramble(uint8_t *pkt)
                {
                    for (int i = 0; i < 160; i++)
                        for (int j = 0; j < 4; j++)
                            pkt[i * 4 + j] = reverseBits(pkt[i * 4 + j]) ^ (scrambling[i] ? 0xFF : 0x00);
                }*/
    }
}