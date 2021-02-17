#pragma once

#include <vector>
#include <cstdint>

template <typename T>
inline bool getBit(T &data, int &bit)
{
    return (data >> bit) & 1;
}

std::vector<uint16_t> repackBits(uint8_t *in, int dst_bits, int skip, int lengthToConvert);