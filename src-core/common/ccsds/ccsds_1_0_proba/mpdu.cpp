#include "mpdu.h"
#include <cstdint>
#include <cmath>

namespace ccsds
{
    namespace ccsds_1_0_proba
    {
        // Parse MPDU from CADU
        MPDU parseMPDU(uint8_t *cadu, bool /*hasVCDUInsertZone*/)
        {
            bool sync_flag = (cadu[8] >> 6) % 2;
            uint16_t first_header_pointer = (cadu[8] % (int)pow(2, 3)) << 8 | cadu[9];
            //first_header_pointer++;
            //if (first_header_pointer == 2046)
            //    first_header_pointer++;

            //shift_right(cadu, 1279, -3);
            //for (int i = 0; i < 1279 - 1; i++)
            // {
            //    cadu[i] = cadu[i] << 7 | cadu[i + 1] >> (8 - 7);
            //}

            return {sync_flag, first_header_pointer, &cadu[10]};
        }

    } // namespace libccsds
} // namespace proba