#include "utils.h"

template <typename T>
inline bool getBit(T &data, int &bit)
{
    return (data >> bit) & 1;
}

std::vector<uint16_t> bytesTo14bits(uint8_t *in, int skip, int lengthToConvert)
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
            shifter = (shifter << 1 | bit) & 0b0001111111111111;
            inShiter++;
            if (inShiter == 14)
            {
                result.push_back(shifter);
                inShiter = 0;
            }
        }
    }

    return result;
}

std::vector<uint16_t> bytesTo13bits(uint8_t *in, int skip, int lengthToConvert)
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
            shifter = (shifter << 1 | bit) & 0b0001111111111111;
            inShiter++;
            if (inShiter == 13)
            {
                result.push_back(shifter);
                inShiter = 0;
            }
        }
    }

    return result;
}

std::vector<uint16_t> bytesTo12bits(uint8_t *in, int skip, int lengthToConvert)
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
            shifter = (shifter << 1 | bit) & 0b0001111111111111;
            inShiter++;
            if (inShiter == 12)
            {
                result.push_back(shifter);
                inShiter = 0;
            }
        }
    }

    return result;
}