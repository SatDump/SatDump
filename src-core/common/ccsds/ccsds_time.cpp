#include "ccsds_time.h"

namespace ccsds
{
    time_t parseCCSDSTime(CCSDSPacket &pkt, int offset, int ms_scale)
    {
        uint16_t days = pkt.payload[0] << 8 | pkt.payload[1];
        uint32_t milliseconds_of_day = pkt.payload[2] << 24 | pkt.payload[3] << 16 | pkt.payload[4] << 8 | pkt.payload[5];

        return (offset + days) * 86400 + milliseconds_of_day / ms_scale;
    }

    double parseCCSDSTimeFull(CCSDSPacket &pkt, int offset, int ms_scale, int us_of_ms_scale)
    {
        uint16_t days = pkt.payload[0] << 8 | pkt.payload[1];
        uint32_t milliseconds_of_day = pkt.payload[2] << 24 | pkt.payload[3] << 16 | pkt.payload[4] << 8 | pkt.payload[5];
        uint16_t microseconds_of_millisecond = pkt.payload[6] << 8 | pkt.payload[7];

        return double(offset + days) * 86400.0 + double(milliseconds_of_day) / double(ms_scale) + double(microseconds_of_millisecond) / double(us_of_ms_scale);
    }

    double parseCCSDSTimeFullRaw(uint8_t *data, int offset, int ms_scale, int us_of_ms_scale)
    {
        uint16_t days = data[0] << 8 | data[1];
        uint32_t milliseconds_of_day = data[2] << 24 | data[3] << 16 | data[4] << 8 | data[5];
        uint16_t microseconds_of_millisecond = data[6] << 8 | data[7];

        return double(offset + days) * 86400.0 + double(milliseconds_of_day) / double(ms_scale) + double(microseconds_of_millisecond) / double(us_of_ms_scale);
    }
}