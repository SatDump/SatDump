#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <complex>
#include "common/codings/dvb-s2/dvbs2.h"

typedef int8_t code_type;

#include "codings/dvb-s2/ldpc/simd.hh"
#include "codings/dvb-s2/ldpc/layered_decoder.hh"
#include "codings/dvb-s2/ldpc/algorithms.hh"
#include "codings/dvb-s2/ldpc/encoder.hh"

namespace dvbs2
{
#if defined(__AVX2__OFF)
    typedef SIMD<code_type, 32> simd_type;
#elif defined(__SSE4_1__)
    typedef SIMD<code_type, 16> simd_type;
#elif defined(__ARM_NEON__) || defined(__ARM_NEON)
    typedef SIMD<code_type, 16> simd_type;
#else
    typedef SIMD<code_type, 1> simd_type;
#endif

    typedef NormalUpdate<simd_type> update_type;
    typedef OffsetMinSumAlgorithm<simd_type, update_type, 2> algorithm_type;

    class BBFrameLDPC
    {
    private:
        simd_type *aligned_buffer;

        LDPCInterface *ldpc;
        LDPCDecoder<simd_type, algorithm_type> decoder;
        LDPCEncoder<int8_t> encoder;
        void init();

    public:
        BBFrameLDPC(dvbs2_framesize_t framesize, dvbs2_code_rate_t rate);
        ~BBFrameLDPC();

        int dataSize()
        {
            return ldpc->data_len();
        }

        int decode(int8_t *frame, int max_trials);
        void encode(uint8_t *frame);

        LDPCInterface *get_instance()
        {
            return ldpc;
        }
    };
}