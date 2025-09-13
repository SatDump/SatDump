#pragma once

#include <cstdint>

namespace satdump
{
    namespace labrador
    {
        enum ldpc_code_t
        {
            /// n=128 k=64 r=1/2
            TC128 = 0,

            /// n=256 k=128 r=1/2
            TC256 = 1,

            /// n=512 k=256 r=1/2
            TC512 = 2,

            /// n=1280 k=1024 r=4/5
            TM1280 = 3,

            /// n=1536 k=1024 r=2/3
            TM1536 = 4,

            /// n=2048 k=1024 r=1/2
            TM2048 = 5,

            /// n=5120 k=4096 r=4/5
            TM5120 = 6,

            /// n=6144 k=4096 r=2/3
            TM6144 = 7,

            /// n=8192 k=4096 r=1/2
            TM8192 = 8,
        };

        struct code_params_t
        {
            // Code type
            ldpc_code_t c;

            /// Block length (number of bits transmitted/received, aka code length).
            uint64_t n;

            /// Data length (number of bits of user information, aka code dimension).
            uint64_t k;

            /// Number of parity bits not transmitted.
            uint64_t punctured_bits;

            /// Sub-matrix size (used in parity check matrix construction).
            uint64_t submatrix_size;

            /// Circulant block size (used in generator matrix construction).
            uint64_t circulant_size;

            /// Sum of the parity check matrix (number of parity check edges).
            uint32_t paritycheck_sum;

            // Almost everything below here can probably vanish once const fn is available,
            // as they can all be represented as simple equations of the parameters above.

            /// Length of the working area required for the bit-flipping decoder.
            /// Equal to n+punctured_bits.
            uint64_t decode_bf_working_len;

            /// Length of the working area required for the message-passing decoder.
            /// Equal to 2 * paritycheck_sum + 3*n + 3*p - 2*k
            uint64_t decode_ms_working_len;

            /// Length of the u8 working area required for the message-passing decoder.
            /// Equal to (n + punctured_bits - k)/8.
            uint64_t decode_ms_working_u8_len;

            /// Length of output required from any decoder.
            /// Equal to (n+punctured_bits)/8.
            uint64_t output_len;

            uint8_t (*ptr_compact_h)[3][4][11];
            uint16_t (*ptr_phi_j_k)[4][26];
            uint64_t *ptr_compact_g;
        };

        code_params_t get_code_params(ldpc_code_t c);
    } // namespace labrador
} // namespace satdump