#pragma once

#include "common/ccsds/ccsds.h"

namespace proba
{
    bool check_proba_crc(ccsds::CCSDSPacket &pkt);
}