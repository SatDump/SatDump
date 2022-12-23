#pragma once

#include "matrix/sparse_matrix.h"

#if defined(__AVX2__)
#include <xmmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>

// Based on gr-ccsds, modified to utilize SIMD
namespace codings
{
    namespace ldpc
    {
        class LDPCDecoderAVX //: public decoder
        {
        public:
            LDPCDecoderAVX(Sparse_matrix pcm);
            ~LDPCDecoderAVX();

            int decode(uint8_t *out, const int8_t *in, int it);

        private:
            int d_pcm_num_cn;
            int d_pcm_num_vn;
            int d_pcm_max_cn_degree;
            int d_pcm_num_edges;

            /* Buffer holding vn values during decode */
            __m256i *d_vns;
            /* Buffer to hold all messages coming from VNs to a single CN */
            __m256i *d_vns_to_cn_msgs;
            /* Buffer to hold the absolute value of messages coming from VNs
             * to CNs */
            __m256i *d_abs_msgs;
            /* Buffer to hold all messages from CNs to VNs containing
             * error correcting values. */
            __m256i *d_cn_to_vn_msgs;

            /* Buffer representing the parity check matrix. Instead of having all offsets of
             * each VN in the VN buffer, we store directly its addresses in the buffer */
            __m256i **d_vn_addr;
            /* Array of row position and degree of each cn */
            int *d_row_pos_deg;

            void generic_cn_kernel(int cn_idx);

            // Used by generic_cn_kernel
            __m256i sign;
            __m256i parity;
            __m256i min, min1, min2;
            __m256i abs_msg;
            __m256i new_msg;
            __m256i msg;
            __m256i equ_wip;
            __m256i equ_min1;
            __m256i to_vn;
            int cn_deg;
            int cn_row_base; // offset to base of corresponding row in the PCM
            int vn_offset;
            int cn_offset; // CN offset in the buffer holding CN to VN messages
        };
    }
}
#endif