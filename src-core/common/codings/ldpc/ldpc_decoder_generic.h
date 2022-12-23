#pragma once

#include "ldpc_decoder.h"

// Based on gr-ccsds
namespace codings
{
    namespace ldpc
    {
        class LDPCDecoderGeneric : public LDPCDecoder
        {
        public:
            LDPCDecoderGeneric(Sparse_matrix pcm);
            ~LDPCDecoderGeneric();

            int decode(uint8_t *out, const int8_t *in, int it);

            int simd() { return 1; }

        private:
            int d_pcm_num_cn;
            int d_pcm_num_vn;
            int d_pcm_max_cn_degree;
            int d_pcm_num_edges;

            /* Buffer holding vn values during decode */
            int16_t *d_vns;
            /* Buffer to hold all messages coming from VNs to a single CN */
            int16_t *d_vns_to_cn_msgs;
            /* Buffer to hold the absolute value of messages coming from VNs
             * to CNs */
            int16_t *d_abs_msgs;
            /* Buffer to hold all messages from CNs to VNs containing
             * error correcting values. */
            int16_t *d_cn_to_vn_msgs;

            /* Buffer representing the parity check matrix. Instead of having all offsets of
             * each VN in the VN buffer, we store directly its addresses in the buffer */
            int16_t **d_vn_addr;
            /* Array of row position and degree of each cn */
            int *d_row_pos_deg;

            void generic_cn_kernel(int cn_idx);

            // Used by generic_cn_kernel
            int16_t sign;
            int16_t parity;
            uint16_t min, min1, min2;
            uint16_t abs_msg;
            int16_t new_msg, msg;
            int16_t abs_mask;
            int16_t equ_min1;
            int16_t to_vn;
            int cn_deg;
            int cn_row_base; // offset to base of corresponding row in the PCM
            int vn_offset;
            int cn_offset; // CN offset in the buffer holding CN to VN messages
        };
    }
}
