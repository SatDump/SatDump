#include "mpdu.h"
#include <cstdint>
#include <cmath>

namespace ccsds
{
    namespace ccsds_tm
    {
        // Parse MPDU from CADU
        MPDU parseMPDU(uint8_t *cadu, bool hasVCDUInsertZone, int insertZoneSize, int mpdu_insert_zone)
        {
            // bool sync_flag = (cadu[8] >> 6) % 2;
            uint16_t first_header_pointer = (cadu[hasVCDUInsertZone ? (8 + insertZoneSize) : 8] & 0b111) << 8 | cadu[hasVCDUInsertZone ? (9 + insertZoneSize) : 9];

            return {first_header_pointer, &cadu[(hasVCDUInsertZone ? (10 + insertZoneSize) : 10) + mpdu_insert_zone]};
        }

    } // namespace libccsds
} // namespace proba