#pragma once

#include <ctime>
#include "common/ccsds/ccsds.h"

namespace jason3
{
    inline time_t parseJasonTime(ccsds::CCSDSPacket &pkt)
    {
        int days = pkt.payload[0] << 8 | pkt.payload[1];
        uint32_t seconds_of_day = pkt.payload[2] << 24 | pkt.payload[3] << 16 | pkt.payload[4] << 8 | pkt.payload[5];

        return ((days * 7) + 3657) * 86400 + seconds_of_day;
    }
}