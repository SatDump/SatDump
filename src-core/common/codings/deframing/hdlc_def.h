#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include "common/codings/crc/crc_generic.h"

namespace ax25
{

    class HDLCDeframer
    {
    private:
        const size_t d_len_min;
        const size_t d_len_max;

        int bitpos = 0;
        int pos_in_pkt = 0;
        uint8_t *wip_pkt;

        int consecutive_ones = 0;

        codings::crc::GenericCRC crc_ccitt = codings::crc::GenericCRC(16, 0x1021, 0xFFFF, 0xFFFF, true, true);

    public:
        HDLCDeframer(int length_min, int length_max);
        ~HDLCDeframer();

        std::vector<std::vector<uint8_t>> work(uint8_t *buffer, int size);
    };
}