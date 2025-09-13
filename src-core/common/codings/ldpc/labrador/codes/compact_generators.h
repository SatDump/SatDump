#pragma once

#include <cstdint>

namespace satdump
{
    namespace labrador
    {
        // Note the X * Y length of each of these arrays:
        // X is the number of blocks vertically (so X * circulant_size is equal k),
        // Y is the number of u64 required for all the bits in a row (so Y*32 is equal n-k).
        //
        // We don't shape the arrays because this way simplifies the type signature of the functions
        // that use them.

        /// Compact generator matrix for the TC128 code
        ///
        /// P is 64x64, `circulant_size`=16, so we have 4 blocks of 1 u64 per row
        extern uint64_t TC128_G[4 * 1];

        /// Compact generator matrix for the TC256 code
        ///
        /// P is 128x128, `circulant_size`=32, so we have 4 blocks of 2 u64 per row
        extern uint64_t TC256_G[4 * 2];

        /// Compact generator matrix for the TC512 code
        ///
        /// P is 256x256, `circulant_size`=64, so we have 4 blocks of 4 u64 per row
        extern uint64_t TC512_G[4 * 4];

        /// Compact generator matrix for the TM1280 code
        ///
        /// P is 1024x256, `circulant_size`=32, so we have 32 blocks of 4 u64 per row
        extern uint64_t TM1280_G[32 * 4];

        /// Compact generator matrix for the TM1536 code
        ///
        /// P is 1024x512, `circulant_size`=64, so we have 16 blocks of 8 u64 per row
        extern uint64_t TM1536_G[16 * 8];

        /// Compact generator matrix for the TM2048 code
        ///
        /// P is 1024x1024, `circulant_size`=128, so we have 8 blocks of 16 u64 per row
        extern uint64_t TM2048_G[8 * 16];

        /// Compact generator matrix for the TM5120 code
        ///
        /// P is 4096x1024, `circulant_size`=128, so we have 32 blocks of 16 u64 per row
        extern uint64_t TM5120_G[32 * 16];

        /// Compact generator matrix for the TM6144 code
        ///
        /// P is 4096x2048, `circulant_size`=256, so we have 16 blocks of 32 u64 per row
        extern uint64_t TM6144_G[16 * 32];

        /// Compact generator matrix for the TM8192 code
        ///
        /// P is 4096x4096, `circulant_size`=512, so we have 8 blocks of 64 u64 per row
        extern uint64_t TM8192_G[8 * 64];
    } // namespace labrador
} // namespace satdump