#pragma once

#include <cstdint>
#include <cstring>

class RepackBitsByte
{
private:
    uint8_t byteToWrite = 0;
    int inByteToWrite = 0;

public:
    int work(uint8_t *in, int length, uint8_t *out);
};