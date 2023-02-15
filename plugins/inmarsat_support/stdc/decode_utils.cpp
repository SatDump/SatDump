#include "decode_utils.h"
#include <cstring>

#include <iostream>

namespace inmarsat
{
    namespace stdc
    {
        // Syncword. Copied from Scytale-C as I was unable to find it somewhere else
        const uint8_t syncword[] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0,
                                    1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0,
                                    0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1,
                                    0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0};

        int compute_frame_match(int8_t *syms, bool &inverted)
        {
            int match_nrm = 0;
            int match_inv = 0;

            for (int i = 0; i < ENCODED_FRAME_SIZE / 162; i++)
            {
                if (syncword[i] ^ (syms[i * 162 + 0] > 0))
                    match_inv++;
                else
                    match_nrm++;

                if (syncword[i] ^ (syms[i * 162 + 1] > 0))
                    match_inv++;
                else
                    match_nrm++;
            }

            if (match_inv > match_nrm)
                inverted = true;
            else
                inverted = false;

            return match_inv > match_nrm ? match_inv : match_nrm;
        }

        void depermute(int8_t *in, int8_t *out)
        {
            for (int i = 0; i < 64; i++)
                memcpy(&out[i * 162], &in[(((i * 23) % 64) & 0x3F) * 162], 162);
        }

        void deinterleave(int8_t *in, int8_t *out)
        {
            for (int row = 0; row < 64; row++)
                for (int col = 0; col < 160; col++)
                    out[col * 64 + row] = in[row * 162 + col + 2];
        }

        const uint8_t scrambling[] = {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
                                      1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0,
                                      1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1,
                                      0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0,
                                      1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0,
                                      1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1,
                                      0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0,
                                      0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0,
                                      1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0,
                                      1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1,
                                      0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0,
                                      0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0,
                                      1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0,
                                      0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1,
                                      0, 0, 1, 1, 1, 0};

        uint8_t reverseBits(uint8_t byte)
        {
            byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
            byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
            byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
            return byte;
        }

        void descramble(uint8_t *pkt)
        {
            for (int i = 0; i < 160; i++)
                for (int j = 0; j < 4; j++)
                    pkt[i * 4 + j] = reverseBits(pkt[i * 4 + j]) ^ (scrambling[i] ? 0xFF : 0x00);
        }
    }
}