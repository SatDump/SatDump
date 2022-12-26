#pragma once

#include <cstdint>
#include "ldpc_decoder.h"

namespace codings
{
    namespace ldpc
    {
        enum ldpc_rate_t
        {
            RATE_1_2,
            RATE_2_3,
            RATE_4_5,
            RATE_7_8,
        };

        ldpc_rate_t ldpc_rate_from_string(std::string str);

        class CCSDSLDPC
        {
        private:
            ldpc_rate_t d_rate;
            int d_block_size;

            int8_t *depunc_buffer_in;
            uint8_t *depunc_buffer_ou;

            int d_codeword_size;
            int d_frame_size;
            int d_data_size;
            int d_is_geneneric;
            int d_simd;
            int d_M;

            int d_corr_errors;

            LDPCDecoder *ldpc_decoder;

            void init_dec(Sparse_matrix pcm);

        public:
            CCSDSLDPC(ldpc_rate_t rate, int block_size);
            ~CCSDSLDPC();

            // Get specifics of the current code
            int frame_length() { return d_frame_size; }
            int codeword_length() { return d_codeword_size; }
            int data_length() { return d_data_size; }
            int simd() { return d_simd; }

            // Decode a LDPC codeword, takes soft-bits floats in, outputs bytes. Retuns corrected bits
            int decode(int8_t *codeword, uint8_t *frame, int iterations = 10);
        };
    }
}