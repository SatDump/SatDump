#pragma once

#include <cstdint>
#include <vector>

namespace ccsds
{
    namespace ccsds_aos
    {
        // Struct representing a M-PDU
        struct MPDU
        {
            uint16_t first_header_pointer;
            uint8_t *data;
        };

        // Parse MPDU from CADU
        MPDU parseMPDU(uint8_t *cadu, bool hasVCDUInsertZone = false, int insertZoneSize = 2);

    } // namespace libccsds
}