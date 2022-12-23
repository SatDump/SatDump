#pragma once

#include <cstdint>
#include "ldpc_decoder.h"
#include "ldpc_decoder_sse.h"
#include "ldpc_decoder_avx.h"
#include "common/cpu_features.h"

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
            int d_simd;
            int d_M;

            int d_corr_errors;

            cpu_features::cpu_features_t cpu_caps;

            LDPCDecoder *ldpc_decoder;
#if defined(__SSE4_1__)
            LDPCDecoderSSE *ldpc_decoder_sse;
#endif
#if defined(__AVX2__)
            LDPCDecoderAVX *ldpc_decoder_avx;
#endif

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