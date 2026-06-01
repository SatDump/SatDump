#include "m10.h"
#include <cstdint>
#include <cstring>

namespace satdump
{
    namespace radiosonde
    {
        void m10_manchester_decode(uint8_t *frmi, int ilen, uint8_t *frmo)
        {
            for (int i = 0; i < ilen * 8; i += 2)
            {
                uint8_t bits = (frmi[i / 8] >> (6 - (i % 8))) & 0b11;
                frmo[i / 16] = frmo[i / 16] << 1 | (bits & 0b01 ? 1 : 0);
            }
        }

        void m10_frame_descramble(uint8_t *frm)
        {
            uint8_t tmp, topbit = 0;

            for (int i = 0; i < 104; i++)
            {
                tmp = frm[i] << 7;
                frm[i] ^= 0xFF ^ (topbit | frm[i] >> 1);
                topbit = tmp;
            }
        }

        bool m10_frame_crccheck(M10Frame *frm)
        {
            if (frm->len > (M10_MAX_DATA_LEN + 2))
                return false;

            const uint8_t *raw_frame = (uint8_t *)&frm->len;
            const uint8_t *crc_ptr = (uint8_t *)&frm->len + frm->len - 1;
            const uint16_t expected = crc_ptr[0] << 8 | crc_ptr[1];

            uint16_t crc = 0;

            for (; raw_frame < crc_ptr; raw_frame++)
            {
                uint16_t c = crc;
                uint8_t b = *raw_frame;

                int c0, c1, t, t6, t7, s;
                c1 = c & 0xFF;
                // B
                b = (b >> 1) | ((b & 1) << 7);
                b ^= (b >> 2) & 0xFF;
                // A1
                t6 = (c & 1) ^ ((c >> 2) & 1) ^ ((c >> 4) & 1);
                t7 = ((c >> 1) & 1) ^ ((c >> 3) & 1) ^ ((c >> 5) & 1);
                t = (c & 0x3F) | (t6 << 6) | (t7 << 7);
                // A2
                s = (c >> 7) & 0xFF;
                s ^= (s >> 2) & 0xFF;
                c0 = b ^ t ^ s;
                crc = ((c1 << 8) | c0) & 0xFFFF;
            }

            return crc == expected;
        }
    } // namespace radiosonde
} // namespace satdump