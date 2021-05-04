#include "ccsds.h"
#include <cstdint>
#include <cmath>
#include <cstring>

namespace ccsds
{
    namespace ccsds_1_0_jason
    {
        CCSDSHeader::CCSDSHeader()
        {
        }

        CCSDSHeader::CCSDSHeader(uint8_t *rawi)
        {
            std::memcpy(raw, rawi, 6);
        }

        // Parse CCSDS header
        CCSDSHeader parseCCSDSHeader(uint8_t *header)
        {
            CCSDSHeader headerObj(header);
            headerObj.version = header[0] >> 5;
            headerObj.type = (header[0] >> 4) % 2;
            headerObj.secondary_header_flag = (header[0] >> 3) % 2;
            headerObj.apid = (header[0] % (int)pow(2, 3)) << 8 | header[1];
            headerObj.sequence_flag = header[2] >> 6;
            headerObj.packet_sequence_count = (header[2] % (int)pow(2, 6)) << 8 | header[3];
            headerObj.packet_length = header[4] << 8 | header[5];
            return headerObj;
        }
    } // namespace libccsds
} // namespace proba