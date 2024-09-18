#include "mpdu.h"
#include <cstdint>
#include <cmath>

namespace ccsds
{
    namespace ccsds_aos
    {
        // Parse MPDU from CADU
        MPDU parseMPDU(uint8_t *cadu, bool hasVCDUInsertZone, int insertZoneSize)
        {
            uint16_t first_header_pointer = (cadu[hasVCDUInsertZone ? (10 + insertZoneSize) : 10] & 0b111) << 8 | cadu[hasVCDUInsertZone ? (11 + insertZoneSize) : 11];
            return {first_header_pointer, &cadu[hasVCDUInsertZone ? (12 + insertZoneSize) : 12]};
        }

    } // namespace libccsds
}