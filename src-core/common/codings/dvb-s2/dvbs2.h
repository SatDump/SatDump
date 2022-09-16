#pragma once

// Frame sizes
#define FRAME_SIZE_NORMAL 64800
#define FRAME_SIZE_SHORT 16200

namespace dvbs2
{
    enum dvbs2_code_rate_t
    {
        C1_4,
        C1_3,
        C2_5,
        C1_2,
        C3_5,
        C2_3,
        C3_4,
        C4_5,
        C5_6,
        C7_8,
        C8_9,
        C9_10,
    };

    enum dvbs2_framesize_t
    {
        FECFRAME_NORMAL,
        FECFRAME_SHORT,
    };

    enum dvbs2_constellation_t
    {
        MOD_QPSK,
        MOD_8PSK,
        MOD_16APSK,
        MOD_32APSK,
    };
}