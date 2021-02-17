#include "repack_bits_byte.h"

size_t RepackBitsByte::work(uint8_t *in, size_t length, uint8_t *out)
{
    int oo = 0;

    for (int ii = 0; ii < length; ii++)
    {
        byteToWrite = (byteToWrite << 1) | (((int8_t *)in)[ii] & 1);
        inByteToWrite++;

        if (inByteToWrite == 8)
        {
            out[oo++] = byteToWrite;
            inByteToWrite = 0;
        }
    }

    return oo;
}