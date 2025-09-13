#pragma once

#include "codes.h"
#include <cstdint>

namespace satdump
{
    namespace labrador
    {
        struct parity_iter_t
        {
            uint16_t (*phi)[4][26];
            uint8_t (*prototype)[3][4][11];
            uint64_t m;
            uint64_t logmd4; // log2(M/4), used to multiply and divide by M/4
            uint64_t modm;   // the bitmask to AND with to accomplish "mod M", equals m-1
            uint64_t modmd4; // the bitmask to AND with to accomplish "mod M/4", equals (m/4)-1
            uint64_t rowidx;
            uint64_t colidx;
            uint64_t sub_mat_idx;
            uint8_t sub_mat;
            uint64_t sub_mat_val;
            uint64_t check;
        };

        void parity_iter_init(parity_iter_t *i, ldpc_code_t c);

        bool parity_iter_next(parity_iter_t *i, uint64_t *check, uint64_t *var);
    } // namespace labrador
} // namespace satdump