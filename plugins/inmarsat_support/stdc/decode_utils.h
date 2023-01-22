#pragma once

#include <cstdint>

// 640-bytes messages, 1/2 encoded + 2 64-bits syncs = 10368-bits
#define ENCODED_FRAME_SIZE 10368
#define ENCODED_FRAME_SIZE_NOSYNC 10240
#define FRAME_SIZE_BYTES 640

namespace inmarsat
{
    namespace stdc
    {
        int compute_frame_match(int8_t *syms, bool &inverted);
        void depermute(int8_t *in, int8_t *out);
        void deinterleave(int8_t *in, int8_t *out);
        void descramble(uint8_t *pkt);
    }
}