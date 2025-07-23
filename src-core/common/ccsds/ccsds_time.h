#pragma once

/**
 * @file  ccsds_time.h
 * @brief Time header parsing
 */

#include "ccsds.h"
#include <ctime>

namespace ccsds
{
    /**
     * @brief Parse timestamp with 1 second accuracy TODOREWORK maybe phase this out?
     * @param pkt the packet
     * @param offset time offset in seconds
     * @param ms_scale millisecond scale
     * @return time in seconds
     */
    time_t parseCCSDSTime(CCSDSPacket &pkt, int offset = 0, int ms_scale = 1000);

    /**
     * @brief Parse timestamp with full accuracy
     * @param pkt the packet
     * @param offset time offset in seconds
     * @param ms_scale millisecond scale
     * @param ns_of_ms_scale ms of ms scale
     * @return time in seconds
     */
    double parseCCSDSTimeFull(CCSDSPacket &pkt, int offset, int ms_scale = 1000, int ns_of_ms_scale = 1000000);

    /**
     * @brief Parse timestamp with full accuracy, from a raw buffer
     * @param data pointer to parse from
     * @param offset time offset in seconds
     * @param ms_scale millisecond scale
     * @param ns_of_ms_scale ms of ms scale
     * @return time in seconds
     */
    double parseCCSDSTimeFullRaw(uint8_t *data, int offset, int ms_scale = 1000, int ns_of_ms_scale = 1000000);

    /**
     * @brief Parse Coarse timestamp. Used by some EOS instruments on Aqua, from a raw buffer
     * @param data pointer to parse from
     * @param offset time offset in seconds
     * @param ms_scale millisecond scale
     * @return time in seconds
     */
    double parseCCSDSTimeFullRawUnsegmented(uint8_t *data, int offset, double ms_scale);
} // namespace ccsds