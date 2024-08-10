#pragma once

#include <cstdint>
#include <vector>

namespace ccsds
{
    namespace ccsds_tm
    {
        // Struct representing a M-PDU
        struct MPDU
        {
            // bool sync_flag;
            uint16_t first_header_pointer;
            uint8_t *data;
        };

        // Parse MPDU from CADU
        MPDU parseMPDU(uint8_t *cadu, bool hasVCDUInsertZone = false, int insertZoneSize = 2, int mpdu_insert_zone = 0);

    } // namespace libccsds
} // namespace proba