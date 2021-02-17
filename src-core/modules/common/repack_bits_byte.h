#pragma once

#include <cstdint>
#include <cstring>

class RepackBitsByte
{
private:
    uint8_t byteToWrite;
    int inByteToWrite = 0;

public:
    size_t work(uint8_t *in, size_t length, uint8_t *out);
};