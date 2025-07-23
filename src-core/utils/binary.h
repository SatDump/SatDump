#pragma once

#include <cstdint>
#include <vector>

namespace satdump
{
    template <typename T>
    inline bool getBit(T &data, int &bit)
    {
        return (data >> bit) & 1;
    }

    template <typename T>
    std::vector<uint8_t> unsigned_to_bitvec(T v)
    {
        std::vector<uint8_t> c;
        for (int s = (sizeof(T) * 8) - 1; s >= 0; s--)
            c.push_back((v >> s) & 1);
        return c;
    }

    template <typename T>
    std::vector<T> oversample_vector(std::vector<T> data, int oversampling)
    {
        std::vector<T> r;
        for (T &v : data)
            for (int i = 0; i < oversampling; i++)
                r.push_back(v);
        return r;
    }

    template <typename T>
    T swap_endian(T u)
    {
        union {
            T u;
            unsigned char u8[sizeof(T)];
        } source, dest;

        source.u = u;

        for (size_t k = 0; k < sizeof(T); k++)
            dest.u8[k] = source.u8[sizeof(T) - k - 1];

        return dest.u;
    }

    inline std::vector<float> double_buffer_to_float(double *ptr, int size)
    {
        std::vector<float> ret;
        for (int i = 0; i < size; i++)
            ret.push_back(ptr[i]);
        return ret;
    }

    uint8_t reverseBits(uint8_t byte);

    uint16_t reverse16Bits(uint16_t v);
} // namespace satdump