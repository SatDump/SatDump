#pragma once

#include <cstdint>
namespace satdump
{
    namespace official
    {
        // TODOREWORK
        inline int get_i4(uint8_t *buff)
        {
            // if (isbig) return *((int_4 *) buff);
            uint8_t i4[4];
            i4[0] = buff[3];
            i4[1] = buff[2];
            i4[2] = buff[1];
            i4[3] = buff[0];
            return *((int *)i4);
        }

        inline double get_r8(uint8_t *buff)
        {
            // if (isbig) return *((real_8 *)buff);
            unsigned char sw[8];
            sw[0] = buff[7];
            sw[1] = buff[6];
            sw[2] = buff[5];
            sw[3] = buff[4];
            sw[4] = buff[3];
            sw[5] = buff[2];
            sw[6] = buff[1];
            sw[7] = buff[0];
            return *((double *)sw);
        }
    } // namespace official
} // namespace satdump