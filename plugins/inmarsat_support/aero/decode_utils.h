#pragma once

#include <cstdint>
#include <vector>
#include <cstring>

// 640-bytes messages, 1/2 encoded + 2 64-bits syncs = 10368-bits
#define ENCODED_FRAME_SIZE 1200
#define ENCODED_FRAME_SIZE_NOSYNC 1200 - 32
#define FRAME_SIZE_BYTES 150

namespace inmarsat
{
    namespace aero
    {
        int compute_frame_match(int8_t *syms, bool &inverted);
        void deinterleave(int8_t *input, int8_t *output, int cols);
        // void descramble(uint8_t *pkt);
    }
}