#include "vcdu.h"
#include <cstdint>
#include <cmath>

namespace ccsds
{
    namespace ccsds_aos
    {
        // Parse VCDU from CADU
        VCDU parseVCDU(uint8_t *cadu)
        {
            uint8_t version = cadu[4] >> 6;
            uint16_t spacecraft_id = (cadu[4] & 0b111111) << 2 | cadu[5] >> 6;
            uint8_t vcid = cadu[5] % (int)pow(2, 6);
            uint32_t vcdu_counter = cadu[6] << 16 | cadu[7] << 8 | cadu[8];
            bool replay_flag = cadu[9] >> 7;
            return {version, spacecraft_id, vcid, vcdu_counter, replay_flag};
        }
    } // namespace libccsds
}