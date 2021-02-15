#pragma once

#include <cstdint>
#include <vector>
#include "cadu.h"

namespace proba
{
    namespace libccsds
    {
        // Struct representing a M-PDU
        struct MPDU
        {
            bool sync_flag;
            uint16_t first_header_pointer;
            uint8_t *data;
        };

        // Parse MPDU from CADU
        MPDU parseMPDU(uint8_t *cadu, bool hasVCDUInsertZone = false);

    } // namespace libccsds
} // namespace proba