#pragma once

#include <cstdint>
#include <functional>
#include "common/codings/rotation.h"

/*
I had started re-writing my own from scratch, but I will
admit doing the work for what is just a few days/week
most likely felt a bit... Yeah. So I took the quick-route
and adapted the code from https://github.com/dbdexter-dev/meteor_decode.
All credits for the deinterleaving part goes to dbdexter!
*/

#define INTER_MARKER 0x27
#define INTER_MARKER_STRIDE 80
#define INTER_MARKER_INTERSAMPS (INTER_MARKER_STRIDE - 8)

#define INTER_BRANCH_COUNT 36
#define INTER_BRANCH_DELAY 2048

#define INTER_SIZE(x) (x * 10 / 9 + 8)

namespace meteor
{
    class DeinterleaverReader
    {
    private: // Autocorrelator
        static int autocorrelate(phase_t *rotation, int period, uint8_t *hard, int len);

    private: // Deinterleaver
        int8_t _deint[INTER_BRANCH_COUNT * INTER_BRANCH_COUNT * INTER_BRANCH_DELAY] = { 0 };
        int _cur_branch = 0;
        int _offset = 0;

        void deinterleave(int8_t *dst, const int8_t *src, size_t len);
        size_t deinterleave_num_samples(size_t output_count);
        int deinterleave_expected_sync_offset();

    private: // Utils
        int offset = 0;
        int8_t from_prev[INTER_MARKER_STRIDE] = { 0 };
        phase_t rotation;

    public:
        DeinterleaverReader();
        ~DeinterleaverReader();

        int read_samples(std::function<int(int8_t *, size_t)> read, int8_t *dst, size_t len);
    };
}