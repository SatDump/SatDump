#include "decode_utils.h"
#include <cstring>
#include <iostream>

namespace inmarsat
{
    namespace aero
    {
        void deinterleave(int8_t *input, int8_t *output, int cols)
        {
            const int rows = 64;
            int k = 0;
            for (int j = 0; j < cols; j++)
                for (int i = 0; i < rows; i++)
                    output[k++] = input[((i * 27) % rows) * cols + j];
        }

        int depuncture(int8_t *in, uint8_t *out, int shift, int size)
        {
            int oo = 0;
            int actual_shift = shift % 3;

            if (shift > 2)
                out[oo++] = 128;

            for (int i = 0; i < size; i++)
            {
                if ((i + actual_shift) % 3 == 0)
                    out[oo++] = in[i] + 127;
                else if ((i + actual_shift) % 3 == 1)
                {
                    out[oo++] = in[i] + 127;
                    out[oo++] = 128;
                }
                else if ((i + actual_shift) % 3 == 2)
                    out[oo++] = in[i] + 127;
            }

            return oo;
        }

        void unpack_areo_c84_packet(uint8_t *input, uint8_t *voice, uint8_t *blocks)
        {
            // Voice
            int in_voice_byte = 0;
            uint8_t voiceByte;
            int voice_o = 0;

            // Blocks
            int in_block_byte = 0;
            uint8_t blockByte;
            int block_o = 0;

            int bitpos = 0;
            for (int i = 0; i < 341; i++)
            {
                for (int y = 7; y >= 0; y--)
                {
                    uint8_t bit = (input[i] >> y) & 1;

                    int bpos = bitpos % 109;

                    // Voice
                    if (0 < bpos && bpos <= 96)
                    {
                        voiceByte = voiceByte << 1 | bit;
                        in_voice_byte++;

                        if (in_voice_byte == 8)
                        {
                            voice[voice_o++] = voiceByte;
                            in_voice_byte = 0;
                        }
                    }

                    // Blocks
                    if (96 < bpos && bpos <= 109)
                    {
                        blockByte = bit << 7 | blockByte >> 1;
                        in_block_byte++;

                        if (in_block_byte == 8)
                        {
                            blocks[block_o++] = blockByte;
                            in_block_byte = 0;
                        }
                    }

                    bitpos++;
                }
            }
        }

        AmbeDecoder::AmbeDecoder()
        {
            mbe_initMbeParms(&current, &prev, &prev_mp_enhanced);
        }

        void AmbeDecoder::decode(uint8_t *data, int blocks, int16_t *out_audio)
        {
            for (int i = 0; i < blocks; i++)
            {
                uint8_t *ambe_frame = &data[i * 12];

                for (int x = 0; x < 12; x++)
                    for (int i = 0; i < 8; i++)
                        ambe_bits[(x * 8) + (7 - i)] = (ambe_frame[x] >> i) & 1;

                for (int i = 0; i < 96; i++)
                    ambe_fr[rW[i]][rX[i]] = ambe_bits[i];

                mbe_processAmbe4800x3600Frame(&out_audio[i * 160], &errors, &errors2, errorbuffer, ambe_fr, ambe_d, &current, &prev, &prev_mp_enhanced, 1);
            }
        }
    }
}