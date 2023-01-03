#include "mpdu.h"
#include <cstdint>
#include <cmath>

namespace ccsds
{
    namespace ccsds_standard
    {
        // Parse MPDU from CADU
        MPDU parseMPDU(uint8_t *cadu, bool hasVCDUInsertZone, int insertZoneSize, int mpdu_insert_zone)
        {
            bool sync_flag = (cadu[8] >> 6) % 2;
            // uint16_t first_header_pointer = (cadu[8] % (int)pow(2, 3)) << 8 | cadu[9];

            uint16_t first_header_pointer = (cadu[hasVCDUInsertZone ? (8 + insertZoneSize) : 8] % (int)pow(2, 3)) << 8 | cadu[hasVCDUInsertZone ? (9 + insertZoneSize) : 9];

            // first_header_pointer++;
            // if (first_header_pointer == 2046)
            //     first_header_pointer++;

            // shift_right(cadu, 1279, -3);
            // for (int i = 0; i < 1279 - 1; i++)
            //  {
            //     cadu[i] = cadu[i] << 7 | cadu[i + 1] >> (8 - 7);
            // }
            return {sync_flag, first_header_pointer, &cadu[(hasVCDUInsertZone ? (10 + insertZoneSize) : 10) + mpdu_insert_zone]};
        }

    } // namespace libccsds
} // namespace proba