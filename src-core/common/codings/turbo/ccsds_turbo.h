#pragma once

extern "C"
{
#include "libs/deepspace-turbo/libconvcodes.h"
#include "libs/deepspace-turbo/libturbocodes.h"
}
#include <cstdint>

namespace codings
{
    namespace turbo
    {
        enum turbo_base_t
        {
            BASE_223 = 223,
            BASE_446 = 446,
            BASE_892 = 892,
            BASE_1115 = 1115,
        };

        enum turbo_rate_t
        {
            RATE_1_2,
            RATE_1_3,
            RATE_1_4,
            RATE_1_6,
        };

        /*
        CCSDS Turbo Decoder based on :
        https://github.com/khawatkom/gr-ccsds-1
        */
        class CCSDSTurbo
        {
        private:
            int d_base;
            int d_octets;
            turbo_rate_t d_code_type;

            float d_rate;
            int d_info_length;
            int d_encoded_length;

            float d_sigma = 0.707;

            static const int MAX_COMPONENTS = 4;

            int *d_pi;
            const char *d_forward_upper[MAX_COMPONENTS];
            const char *d_forward_lower[MAX_COMPONENTS];
            const char *d_backward;
            t_convcode d_code1;
            t_convcode d_code2;
            t_turbocode d_turbo;

            int puncturing(int k)
            {

                int bit_idx = k % 3;

                // bit 0,3,6,... corresponding to systematic output
                if (!bit_idx)
                    return 1;

                // get block index
                int block_idx = k / 3;

                // on odd blocks puncture second bit
                if (block_idx % 2)
                    return bit_idx != 1;

                // on even blocks puncture third bit
                return bit_idx != 2;
            }

        public:
            CCSDSTurbo(turbo_base_t base, turbo_rate_t type);
            ~CCSDSTurbo();

            // Get specifics of the current code
            int frame_length() { return d_info_length; }
            int codeword_length() { return d_encoded_length; }

            // Set Sigma for decoding
            void set_sigma(float sigma) { d_sigma = sigma; }

            // Encode a Turbo codeword, takes bytes in, output bytes
            void encode(uint8_t *frame, uint8_t *codeword);

            // Decode a Turbo codeword, takes soft-bits floats in, outputs bytes
            void decode(float *codeword, uint8_t *frame, int iterations = 10);
        };
    }
}