#pragma once

#include "matrix/sparse_matrix.h"

namespace codings
{
    namespace ldpc
    {
        namespace ccsds_ar4ja
        {
            enum ar4ja_rate_t
            {
                AR4JA_RATE_1_2,
                AR4JA_RATE_2_3,
                AR4JA_RATE_4_5,
            };

            enum ar4ja_blocksize_t
            {
                AR4JA_BLOCK_1024,
                AR4JA_BLOCK_4096,
                AR4JA_BLOCK_16384,
            };

            codings::ldpc::Sparse_matrix make_ar4ja_code(ar4ja_rate_t rate, ar4ja_blocksize_t k, int *M = nullptr);
        }

        namespace ccsds_78
        {
            codings::ldpc::Sparse_matrix make_r78_code();
        }
    }
}