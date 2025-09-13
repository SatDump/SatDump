#pragma once

#include <cstdint>

namespace satdump
{
    namespace labrador
    {
        // Constants used to define the parity check matrices for the TC codes.
        //
        // This representation mostly mirrors that in CCSDS 231.1-O-1.
        // Each constant represents a single MxM sub-matrix, where M=n/8.
        // * HZ:   All-zero matrix
        // * HI:   Identity matrix
        // * HI|n: nth right circular shift of I, with lower 6 bits for n
        // The two sets of matrices are summed together to handle the case HI + HI|n.
        // The third zero matrix is annoying - without it we have to pass slices and know about
        // lengths in the iterator, which is so much slower that it seems worth the 44-byte-per-code-used
        // extra space used in code memory.
        //

        const uint8_t HZ = 0 << 6;
        const uint8_t HI = 1 << 6;

        /// Compact parity matrix for the TC128 code
        extern uint8_t TC128_H[3][4][11];

        /// Compact parity matrix for the TC256 code
        extern uint8_t TC256_H[3][4][11];

        /// Compact parity matrix for the TC512 code
        extern uint8_t TC512_H[3][4][11];

        // Parity check matrices corresponding to the TM codes.
        //
        // This representation mirrors the definition in CCSDS 131.0-B-1,
        // and can be expanded at runtime to create the actual matrix in memory.
        // Each const represents a single MxM sub-matrix, where M is a function
        // of the information block length and the rate:
        //
        // -----------------------------------------
        // |k     | Rate 1/2 | Rate 2/3 | Rate 4/5 |
        // -----------------------------------------
        // |1024  |      512 |      256 |      128 |
        // |4096  |     2048 |     1024 |      512 |
        // |16384 |     8192 |     4096 |     2048 |
        // -----------------------------------------
        //
        // The HP constant is used for PI_K which goes via the lookup table below, with value K-1.
        // The HI macro is an MxM identity, as previously, and we don't use it with any rotation.
        // The HZ constant is an MxM zero block.
        //
        // Each matrix is defined in three parts which are to be added together mod 2.
        //
        // While it's not super space efficient, to make runtime quicker and easier we write the full
        // prototype matrix for each rate, instead of right-appending the lower-rate matrix dynamically.
        // This costs 198 extra bytes of flash storage but it's well worth it (seriously, earlier versions
        // did not do this and it was a nightmare).
        //
        // The PI_K function is:
        // pi_k(i) = M/4 (( theta_k + floor(4i / M)) mod 4) + (phi_k( floor(4i / M), M ) + i) mod M/4

        const uint8_t HP = 2 << 6;

        /// Compact parity matrix for the rate-1/2 TM codes
        extern uint8_t TM_R12_H[3][4][11];

        /// Compact parity matrix for the rate-2/3 TM codes
        extern uint8_t TM_R23_H[3][4][11];

        /// Compact parity matrix for the rate-4/5 TM codes
        extern uint8_t TM_R45_H[3][4][11];

        /// Theta constants. Looked up against (k-1) from k=1 to k=26.
        extern uint8_t THETA_K[26];

        /// Phi constants for M=128.
        ///
        /// First index is j (from 0 to 3), second index is k-1 (from k=1 to k=26).
        /// These are split up by M because we only need one set for each TM code, so the linker can throw
        /// away the unused constants.
        ///
        /// Ideally the first four would be u8 instead of u16 to save space, but this
        /// complicates actually using them -- the monomorphised generic function
        /// would likely take more text space than just making them all u16 for 104 extra bytes per code.
        extern uint16_t PHI_J_K_M128[4][26];

        /// Phi constants for M=256. See docs for `PHI_J_K_M128`.
        extern uint16_t PHI_J_K_M256[4][26];

        /// Phi constants for M=512. See docs for `PHI_J_K_M128`.
        extern uint16_t PHI_J_K_M512[4][26];

        /// Phi constants for M=1024. See docs for `PHI_J_K_M128`.
        extern uint16_t PHI_J_K_M1024[4][26];

        /// Phi constants for M=2048. See docs for `PHI_J_K_M128`.
        extern uint16_t PHI_J_K_M2048[4][26];

        /// Phi constants for M=4096. See docs for `PHI_J_K_M128`.
        extern uint16_t PHI_J_K_M4096[4][26];

        /// Phi constants for M=8192. See docs for `PHI_J_K_M128`.
        extern uint16_t PHI_J_K_M8192[4][26];
    } // namespace labrador
} // namespace satdump