#include "ccsds_time.h"

namespace ccsds
{
    time_t parseCCSDSTime(CCSDSPacket &pkt, int offset, int ms_scale)
    {
        uint16_t days = pkt.payload[0] << 8 | pkt.payload[1];
        uint32_t milliseconds_of_day = pkt.payload[2] << 24 | pkt.payload[3] << 16 | pkt.payload[4] << 8 | pkt.payload[5];
        //uint16_t milliseconds_of_millisecond = pkt.payload[6] << 8 | pkt.payload[7];

        return (offset + days) * 86400 + milliseconds_of_day / ms_scale;
    }

    int parseCCSDSTimeMillisecondOfMs(CCSDSPacket &pkt, int /*offset*/)
    {
        uint16_t milliseconds_of_millisecond = pkt.payload[6] << 8 | pkt.payload[7];
        return milliseconds_of_millisecond;
    }
}