#pragma once

#include <cstdint>
namespace metopsg
{
    // TODOREWORK common with AWS?
    inline double parseCUC(uint8_t *dat)
    {
        double seconds = dat[1] << 24 | dat[2] << 16 | dat[3] << 8 | dat[4];
        double fseconds = dat[5] << 16 | dat[6] << 8 | dat[7];
        double timestamp = seconds + fseconds / 16777215.0 + 3657 * 24 * 3600;
        if (timestamp > 0)
            return timestamp;
        else
            return -1;
    }
} // namespace metopsg