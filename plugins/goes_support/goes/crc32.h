#pragma once

#include <cstdint>

namespace goes
{
    class CRC32
    {
    private:
        const uint32_t polynomial = 0xEDB88320;
        uint32_t table[256];

    public:
        CRC32()
        {
            for (int i = 0; i < 256; i++)
            {
                uint32_t c = i;
                for (int j = 0; j < 8; j++)
                {
                    if (c & 1)
                        c = polynomial ^ (c >> 1);
                    else
                        c >>= 1;
                }

                table[i] = c;
            }
        }

        uint32_t compute(uint8_t *buf, int len)
        {
            uint32_t c = 0xFFFFFFFF; // Initial value
            for (int i = 0; i < len; ++i)
                c = table[(c & 0xFF) ^ buf[i]] ^ (c >> 8);
            return c ^ 0xFFFFFFFF;
        }
    };
}