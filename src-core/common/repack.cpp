#include "repack.h"

/*
This file contains a bunch of simple bit repacking functions,
for easy-of-use.
They are all byte-aligned, so if you do not want to be,
shifting would have to be done beforehand.
*/

#include <iostream>

int repackBytesTo10bits(uint8_t *bytes, int byte_length, uint16_t *words)
{
    int bpos = 0;
    int wpos = 0;

    // Compute how many we can repack using the "fast" way
    int repack_fast = byte_length - (byte_length % 5);
    int repack_slow = byte_length % 5;

    // Repack what can be repacked fast
    for (int i = 0; i < repack_fast; i += 5)
    {
        words[wpos++] = bytes[bpos + 0] << 2 | bytes[bpos + 1] >> 6;
        words[wpos++] = (bytes[bpos + 1] % 64) << 4 | bytes[bpos + 2] >> 4;
        words[wpos++] = (bytes[bpos + 2] % 16) << 6 | bytes[bpos + 3] >> 2;
        words[wpos++] = (bytes[bpos + 3] % 4) << 8 | bytes[bpos + 4];
        bpos += 5;
    }

    // Repack remaining using a slower method
    uint16_t shifter = 0;
    int inshifter = 0;
    for (int i = 0; i < repack_slow; i++)
    {
        for (int b = 7; b >= 0; b--)
        {
            shifter = (shifter << 1 | ((bytes[bpos] >> b) & 1)) & 1023;
            inshifter++;
            if (inshifter == 10)
            {
                words[wpos++] = shifter;
                inshifter = 0;
            }
        }

        bpos++;
    }

    return wpos;
}

int repackBytesTo12bits(uint8_t *bytes, int byte_length, uint16_t *words)
{
    int bpos = 0;
    int wpos = 0;

    // Compute how many we can repack using the "fast" way
    int repack_fast = byte_length - (byte_length % 3);
    int repack_slow = byte_length % 3;

    // Repack what can be repacked fast
    for (int i = 0; i < repack_fast; i += 3)
    {
        words[wpos++] = bytes[bpos + 0] << 4 | bytes[bpos + 1] >> 4;
        words[wpos++] = (bytes[bpos + 1] & 0b1111) << 8 | bytes[bpos + 2];
        bpos += 3;
    }

    // Repack remaining using a slower method
    uint16_t shifter = 0;
    int inshifter = 0;
    for (int i = 0; i < repack_slow; i++)
    {
        for (int b = 7; b >= 0; b--)
        {
            shifter = (shifter << 1 | ((bytes[bpos] >> b) & 1)) & 4095;
            inshifter++;
            if (inshifter == 12)
            {
                words[wpos++] = shifter;
                inshifter = 0;
            }
        }

        bpos++;
    }

    return wpos;
}

int repackBytesTo13bits(uint8_t *bytes, int byte_length, uint16_t *words)
{
    int bpos = 0;
    int wpos = 0;

    // Compute how many we can repack using the "fast" way
    int repack_fast = byte_length - (byte_length % 13);
    int repack_slow = byte_length % 13;

    // Repack what can be repacked fast
    for (int i = 0; i < repack_fast; i += 13)
    {
        words[wpos++] = bytes[bpos + 0] << 5 | bytes[bpos + 1] >> 3;
        words[wpos++] = (bytes[bpos + 1] & 0b111) << 10 | bytes[bpos + 2] << 2 | bytes[bpos + 3] >> 6;
        words[wpos++] = (bytes[bpos + 3] & 0b111111) << 7 | bytes[bpos + 4] >> 1;
        words[wpos++] = (bytes[bpos + 4] & 0b1) << 12 | bytes[bpos + 5] << 4 | bytes[bpos + 6] >> 4;
        words[wpos++] = (bytes[bpos + 6] & 0b1111) << 9 | bytes[bpos + 7] << 1 | bytes[bpos + 8] >> 7;
        words[wpos++] = (bytes[bpos + 8] & 0b1111111) << 6 | bytes[bpos + 9] >> 2;
        words[wpos++] = (bytes[bpos + 9] & 0b11) << 11 | bytes[bpos + 10] << 3 | bytes[bpos + 11] >> 5;
        words[wpos++] = (bytes[bpos + 11] & 0b11111) << 8 | bytes[bpos + 12];
        bpos += 13;
    }

    // Repack remaining using a slower method
    uint16_t shifter = 0;
    int inshifter = 0;
    for (int i = 0; i < repack_slow; i++)
    {
        for (int b = 7; b >= 0; b--)
        {
            shifter = (shifter << 1 | ((bytes[bpos] >> b) & 1)) & 8191;
            inshifter++;
            if (inshifter == 13)
            {
                words[wpos++] = shifter;
                inshifter = 0;
            }
        }

        bpos++;
    }

    return wpos;
}

int repackBytesTo16bits(uint8_t *bytes, int byte_length, uint16_t *words)
{
    int bpos = 0;
    int wpos = 0;

    // Compute how many we can repack using the "fast" way
    // This also is how we will repack at all
    int repack_fast = byte_length - (byte_length % 2);

    // Repack what can be repacked fast
    for (int i = 0; i < repack_fast; i += 2)
    {
        words[wpos++] = bytes[bpos + 0] << 8 | bytes[bpos + 1];
        bpos += 2;
    }

    return wpos;
}

int repackBytesTo17bits(uint8_t *bytes, int byte_length, uint32_t *words)
{
    int bpos = 0;
    int wpos = 0;

    // Repack remaining using a slower method
    uint32_t shifter = 0;
    int inshifter = 0;
    for (int i = 0; i < byte_length; i++)
    {
        for (int b = 7; b >= 0; b--)
        {
            shifter = (shifter << 1 | ((bytes[bpos] >> b) & 1)) & 0b11111111111111111;
            inshifter++;
            if (inshifter == 17)
            {
                words[wpos++] = shifter;
                inshifter = 0;
            }
        }

        bpos++;
    }

    return wpos;
}

int repackBytesTo20bits(uint8_t *bytes, int byte_length, uint32_t *words)
{
    int bpos = 0;
    int wpos = 0;

    // Repack remaining using a slower method
    uint32_t shifter = 0;
    int inshifter = 0;
    for (int i = 0; i < byte_length; i++)
    {
        for (int b = 7; b >= 0; b--)
        {
            shifter = (shifter << 1 | ((bytes[bpos] >> b) & 1)) & 0b11111111111111111111;
            inshifter++;
            if (inshifter == 20)
            {
                words[wpos++] = shifter;
                inshifter = 0;
            }
        }

        bpos++;
    }

    return wpos;
}

int repackBytesTo14bits(uint8_t *bytes, int byte_length, uint16_t *words)
{
    int bpos = 0;
    int wpos = 0;

    // Repack remaining using a slower method
    uint16_t shifter = 0;
    int inshifter = 0;
    for (int i = 0; i < byte_length; i++)
    {
        for (int b = 7; b >= 0; b--)
        {
            shifter = (shifter << 1 | ((bytes[bpos] >> b) & 1)) & 0b11111111111111;
            inshifter++;
            if (inshifter == 14)
            {
                words[wpos++] = shifter;
                inshifter = 0;
            }
        }

        bpos++;
    }

    return wpos;
}

void shift_array_left(uint8_t *in, int byte_length, int shift, uint8_t *out)
{
    for (int i = 0; i < byte_length; i++)
        out[i] = in[i + 0] << shift | in[i + 1] >> (8 - shift);
}