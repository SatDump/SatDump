#include "encoder.h"

namespace satdump
{
    namespace labrador
    {
        void encode_parity_u8(code_params_t c, uint8_t *data)
        {
            uint64_t k = c.k;
            uint64_t r = c.n - c.k;
            uint64_t b = c.circulant_size;
            uint64_t *gc = c.ptr_compact_g;
            uint64_t row_len = r / 64;
            uint8_t *parity = data + (c.k / 8);

            for (uint64_t x = 0; x < r / 8; x++)
                parity[x] = 0;

            // For each rotation of the generator circulants
            for (uint64_t offset = 0; offset < b; offset++)
            {
                // For each row of circulants
                for (uint64_t crow = 0; crow < (k / b); crow++)
                {
                    // Data bit (row of full generator matrix)
                    uint64_t bit = crow * b + offset;
                    if (((data[bit / 8] >> (7 - (bit % 8))) & 1) == 1)
                    {
                        // If bit is set, XOR the generator constant in
                        for (uint64_t idx = 0; idx < row_len; idx++)
                        {
                            uint64_t &circ = gc[crow * row_len + idx];
                            parity[idx * 8 + 7] ^= (circ >> 0) & 0xFF;
                            parity[idx * 8 + 6] ^= (circ >> 8) & 0xFF;
                            parity[idx * 8 + 5] ^= (circ >> 16) & 0xFF;
                            parity[idx * 8 + 4] ^= (circ >> 24) & 0xFF;
                            parity[idx * 8 + 3] ^= (circ >> 32) & 0xFF;
                            parity[idx * 8 + 2] ^= (circ >> 40) & 0xFF;
                            parity[idx * 8 + 1] ^= (circ >> 48) & 0xFF;
                            parity[idx * 8 + 0] ^= (circ >> 56) & 0xFF;
                        }
                    }
                }

                // Now simulate the right-rotation of the generator by left-rotating the parity
                for (uint64_t block = 0; block < (r / b); block++)
                {
                    uint8_t *parityblock = parity + block * (b / 8);
                    uint8_t carry = parityblock[0] >> 7;
                    for (uint64_t i = 0; i < b / 8; i++)
                    {
                        uint8_t &x = parityblock[((b / 8) - 1) - i];
                        uint8_t c = x >> 7;
                        x = (x << 1) | carry;
                        carry = c;
                    }
                }
            }
        }

        void encode_parity_u32(code_params_t c, uint8_t *data)
        {
            uint64_t k = c.k;
            uint64_t r = c.n - c.k;
            uint64_t b = c.circulant_size;
            uint64_t *gc = c.ptr_compact_g;
            uint64_t row_len = r / 64;
            uint8_t *parity = data + (c.k / 8);

            for (uint64_t x = 0; x < r / 8; x++)
                parity[x] = 0;

            // For each rotation of the generator circulants
            for (uint64_t offset = 0; offset < b; offset++)
            {
                // For each row of circulants
                for (uint64_t crow = 0; crow < (k / b); crow++)
                {
                    // Data bit (row of full generator matrix)
                    uint64_t bit = crow * b + offset;
                    if (((data[bit / 8] >> (7 - (bit % 8))) & 1) == 1)
                    {
                        // If bit is set, XOR the generator constant in
                        for (uint64_t idx = 0; idx < row_len; idx++)
                        {
                            uint64_t &circ = gc[crow * row_len + idx];
                            ((uint32_t *)parity)[idx * 2 + 1] ^= (circ >> 0) & 0xFFFFFFFF;
                            ((uint32_t *)parity)[idx * 2 + 0] ^= (circ >> 32) & 0xFFFFFFFF;
                        }
                    }
                }

                // Now simulate the right-rotation of the generator by left-rotating the parity
                if (b >= 32)
                {
                    for (uint64_t block = 0; block < (r / b); block++)
                    {
                        uint32_t *parityblock = ((uint32_t *)parity) + block * (b / 32);
                        uint32_t carry = parityblock[0] >> 31;
                        for (uint64_t i = 0; i < b / 32; i++)
                        {
                            uint32_t &x = parityblock[((b / 32) - 1) - i];
                            uint32_t c = x >> 31;
                            x = (x << 1) | carry;
                            carry = c;
                        }
                    }
                }
                else if (b == 16)
                {
                    // For small blocks we must rotate inside each parity word instead
                    for (uint64_t i = 0; i < (r / 32); i++)
                    {
                        uint32_t &x = ((uint32_t *)parity)[i];
                        uint32_t block1 = x & 0xFFFF0000;
                        uint32_t block2 = x & 0x0000FFFF;
                        x = (((block1 << 1) | (block1 >> 15)) & 0xFFFF0000) | (((block2 << 1) | (block2 >> 15)) & 0x0000FFFF);
                    }
                }
            }

            // Need to compensate for endianness
            for (uint64_t i = 0; i < (r / 32); i++)
            {
                uint8_t v1 = parity[i * 4 + 0];
                uint8_t v2 = parity[i * 4 + 1];
                uint8_t v3 = parity[i * 4 + 2];
                uint8_t v4 = parity[i * 4 + 3];
                parity[i * 4 + 0] = v4;
                parity[i * 4 + 1] = v3;
                parity[i * 4 + 2] = v2;
                parity[i * 4 + 3] = v1;
            }
        }
    } // namespace labrador
} // namespace satdump