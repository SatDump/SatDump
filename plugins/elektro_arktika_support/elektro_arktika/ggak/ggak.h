#pragma once

/**
 * @file ggak.h
 */

#include "common/big_endian.h"
#include <cstdint>

namespace elektro_arktika
{
    namespace ggak
    {
#ifdef _WIN32
#pragma pack(push, 1)
#endif
        struct GGAKFrame
        {
            uint8_t sync[4];
            uint8_t id;
            be_uint16_t master_counter;
            be_uint16_t channel_counter;
            be_uint32_t timestamp;
            uint8_t payload[209];
            be_uint16_t crc;
        }
#ifdef _WIN32
        ;
#else
        __attribute__((packed));
#endif

        double getTimestamp(GGAKFrame *frm);
        bool checkCRC(GGAKFrame *frm);
    } // namespace ggak
} // namespace elektro_arktika