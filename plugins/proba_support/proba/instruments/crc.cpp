#include "crc.h"
#include "common/codings/crc/crc_generic.h"

namespace proba
{
    codings::crc::GenericCRC crc_check(16, 0x0001, 0x0000, 0x0000, false, false);

    bool check_proba_crc(ccsds::CCSDSPacket &pkt)
    {
        // Check CRC!
        std::vector<uint8_t> full_payload;
        full_payload.insert(full_payload.end(), pkt.header.raw, pkt.header.raw + 6);
        full_payload.insert(full_payload.end(), pkt.payload.begin(), pkt.payload.end() - 2);
        uint64_t crc_frm = pkt.payload[pkt.payload.size() - 2] << 8 | pkt.payload[pkt.payload.size() - 1];
        uint64_t crc_com = crc_check.compute(full_payload.data(), full_payload.size());
        full_payload.clear();

        return crc_frm != crc_com;
    }
}