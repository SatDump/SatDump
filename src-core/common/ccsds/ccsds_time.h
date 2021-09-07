#pragma once

#include <ctime>
#include "ccsds.h"

namespace ccsds
{
    time_t parseCCSDSTime(CCSDSPacket &pkt, int offset = 0, int ms_scale = 1000);                               // Parse timestamp with 1 second accuracy
    double parseCCSDSTimeFull(CCSDSPacket &pkt, int offset, int ms_scale = 1000, int ns_of_ms_scale = 1000);    // Parse timestamp with full accuracy
    double parseCCSDSTimeFullRaw(uint8_t *data, int offset, int ms_scale = 1000, int ns_of_ms_scale = 1000); // Parse timestamp with full accuracy, from a raw buffer
}