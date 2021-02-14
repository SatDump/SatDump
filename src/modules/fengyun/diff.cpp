#include "diff.h"

void FengyunDiff::work(uint8_t *in, size_t len, uint8_t *out)
{
    int oo = 0;

    // Process all given samples
    for (size_t ii = 0; ii < len; ii++)
    {
        uint8_t &sample = in[ii];

        // Push new sample into buffer
        buffer[0] = buffer[1];
        buffer[1] = sample;

        // We need at least 2
        if (inBuf < 2)
        {
            inBuf++;
            continue;
        }

        // Perform differential decoding
        Xin_1 = buffer[0] & 0x02;
        Yin_1 = buffer[0] & 0x01;
        Xin = buffer[1] & 0x02;
        Yin = buffer[1] & 0x01;
        if (((Xin >> 1) ^ Yin) == 1)
        {
            Xout = (Yin_1 ^ Yin);
            Yout = (Xin_1 ^ Xin);
            out[oo++] = (Xout << 1) + (Yout >> 1);
        }
        else
        {
            Xout = (Xin_1 ^ Xin);
            Yout = (Yin_1 ^ Yin);
            out[oo++] = (Xout + Yout);
        }
    }
}