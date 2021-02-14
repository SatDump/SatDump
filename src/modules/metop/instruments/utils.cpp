//#include "utils.h"
#include <cmath>
#include <vector>
#include <cstdint>

template <typename T>
inline bool getBit(T &data, int &bit)
{
    return (data >> bit) & 1;
}

std::vector<uint16_t> repackBits(uint8_t *in, int dst_bits, int skip, int lengthToConvert)
{
    std::vector<uint16_t> result;
    uint16_t shifter;
    int inShiter = 0;
    for (int i = 0; i < lengthToConvert; i++)
    {
        for (int b = 7; b >= 0; b--)
        {
            if (--skip > 0)
                continue;

            bool bit = getBit<uint8_t>(in[i], b);
            shifter = (shifter << 1 | bit)  % (int)pow(2, dst_bits);
            inShiter++;
            if (inShiter == dst_bits)
            {
                result.push_back(shifter);
                inShiter = 0;
            }
        }
    }

    return result;
}