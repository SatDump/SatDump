#pragma once

#include <ctime>
#include "ccsds.h"

namespace ccsds
{
    time_t parseCCSDSTime(CCSDSPacket &pkt, int offset = 0, int ms_scale = 1000);
    int parseCCSDSTimeMillisecondOfMs(CCSDSPacket &pkt, int offset = 0);
}