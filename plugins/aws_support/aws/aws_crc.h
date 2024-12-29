#pragma once

#include "common/codings/crc/crc_generic.h"
#include "common/ccsds/ccsds.h"

namespace aws
{
    class AWSCRC
    {
    private:
        codings::crc::GenericCRC crc_ccitt = codings::crc::GenericCRC(16, 0x1021, 0xFFFF, 0x0000, false, false);

    public:
        bool check(ccsds::CCSDSPacket &pkt)
        {
            std::vector<uint8_t> check2;
            check2.insert(check2.end(), pkt.header.raw, pkt.header.raw + 6);
            check2.insert(check2.end(), pkt.payload.begin(), pkt.payload.end());
            uint16_t crc = check2[check2.size() - 2] << 8 | check2[check2.size() - 1];
            return crc == crc_ccitt.compute(check2.data(), check2.size() - 2);
        }
    };
}