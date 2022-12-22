#include "make_ccsds.h"
#include <cassert>

// Ported over from :
// https://github.com/daniestevez/ldpc-toolbox/blob/main/src/codes/ccsds.rs
namespace codings
{
    namespace ldpc
    {
        namespace ccsds_ar4ja
        {
            void mtx_toggle(codings::ldpc::Sparse_matrix &m, size_t row, size_t col)
            {
                if (m.at(row, col))
                    m.rm_connection(row, col);
                else
                    m.add_connection(row, col);
            }

            enum ar4ja_M_t
            {
                M128,
                M256,
                M512,
                M1024,
                M2048,
                M4096,
                M8192,
            };

            int mlog2(ar4ja_M_t v)
            {
                switch (v)
                {
                case M128:
                    return 7;
                case M256:
                    return 8;
                case M512:
                    return 9;
                case M1024:
                    return 10;
                case M2048:
                    return 11;
                case M4096:
                    return 12;
                case M8192:
                    return 13;
                }
                return 0;
            }

            ar4ja_M_t get_m(ar4ja_rate_t rate, ar4ja_blocksize_t k)
            {
                if (k == AR4JA_BLOCK_1024)
                {
                    if (rate == AR4JA_RATE_1_2)
                        return M512;
                    else if (rate == AR4JA_RATE_2_3)
                        return M256;
                    else if (rate == AR4JA_RATE_4_5)
                        return M128;
                }
                else if (k == AR4JA_BLOCK_4096)
                {
                    if (rate == AR4JA_RATE_1_2)
                        return M2048;
                    else if (rate == AR4JA_RATE_2_3)
                        return M1024;
                    else if (rate == AR4JA_RATE_4_5)
                        return M512;
                }
                else if (k == AR4JA_BLOCK_16384)
                {
                    if (rate == AR4JA_RATE_1_2)
                        return M8192;
                    else if (rate == AR4JA_RATE_2_3)
                        return M4096;
                    else if (rate == AR4JA_RATE_4_5)
                        return M2048;
                }
                return M512;
            }

            uint8_t THETA_K[26] = {3, 0, 1, 2, 2, 3, 0, 1, 0, 1, 2, 0, 2, 3, 0, 1, 2, 0, 1, 2, 0, 1, 2, 1, 2, 3};

            // Table 7-3 and 7-4 in [1]
            size_t mtheta(size_t k)
            {
                assert(k <= 26);
                return THETA_K[k - 1];
            }

            // Table 7-3 and 7-4 in [1]
            size_t PHI_K[4][26][7] = {
                // j = 0
                {
                    {1, 59, 16, 160, 108, 226, 1148},
                    {22, 18, 103, 241, 126, 618, 2032},
                    {0, 52, 105, 185, 238, 404, 249},
                    {26, 23, 0, 251, 481, 32, 1807},
                    {0, 11, 50, 209, 96, 912, 485},
                    {10, 7, 29, 103, 28, 950, 1044},
                    {5, 22, 115, 90, 59, 534, 717},
                    {18, 25, 30, 184, 225, 63, 873},
                    {3, 27, 92, 248, 323, 971, 364},
                    {22, 30, 78, 12, 28, 304, 1926},
                    {3, 43, 70, 111, 386, 409, 1241},
                    {8, 14, 66, 66, 305, 708, 1769},
                    {25, 46, 39, 173, 34, 719, 532},
                    {25, 62, 84, 42, 510, 176, 768},
                    {2, 44, 79, 157, 147, 743, 1138},
                    {27, 12, 70, 174, 199, 759, 965},
                    {7, 38, 29, 104, 347, 674, 141},
                    {7, 47, 32, 144, 391, 958, 1527},
                    {15, 1, 45, 43, 165, 984, 505},
                    {10, 52, 113, 181, 414, 11, 1312},
                    {4, 61, 86, 250, 97, 413, 1840},
                    {19, 10, 1, 202, 158, 925, 709},
                    {7, 55, 42, 68, 86, 687, 1427},
                    {9, 7, 118, 177, 168, 752, 989},
                    {26, 12, 33, 170, 506, 867, 1925},
                    {17, 2, 126, 89, 489, 323, 270},
                },
                // j = 1
                {
                    {0, 0, 0, 0, 0, 0, 0},
                    {27, 32, 53, 182, 375, 767, 1822},
                    {30, 21, 74, 249, 436, 227, 203},
                    {28, 36, 45, 65, 350, 247, 882},
                    {7, 30, 47, 70, 260, 284, 1989},
                    {1, 29, 0, 141, 84, 370, 957},
                    {8, 44, 59, 237, 318, 482, 1705},
                    {20, 29, 102, 77, 382, 273, 1083},
                    {26, 39, 25, 55, 169, 886, 1072},
                    {24, 14, 3, 12, 213, 634, 354},
                    {4, 22, 88, 227, 67, 762, 1942},
                    {12, 15, 65, 42, 313, 184, 446},
                    {23, 48, 62, 52, 242, 696, 1456},
                    {15, 55, 68, 243, 188, 413, 1940},
                    {15, 39, 91, 179, 1, 854, 1660},
                    {22, 11, 70, 250, 306, 544, 1661},
                    {31, 1, 115, 247, 397, 864, 587},
                    {3, 50, 31, 164, 80, 82, 708},
                    {29, 40, 121, 17, 33, 1009, 1466},
                    {21, 62, 45, 31, 7, 437, 433},
                    {2, 27, 56, 149, 447, 36, 1345},
                    {5, 38, 54, 105, 336, 562, 867},
                    {11, 40, 108, 183, 424, 816, 1551},
                    {26, 15, 14, 153, 134, 452, 2041},
                    {9, 11, 30, 177, 152, 290, 1383},
                    {17, 18, 116, 19, 492, 778, 1790},
                },
                // j = 2
                {
                    {0, 0, 0, 0, 0, 0, 0},
                    {12, 46, 8, 35, 219, 254, 318},
                    {30, 45, 119, 167, 16, 790, 494},
                    {18, 27, 89, 214, 263, 642, 1467},
                    {10, 48, 31, 84, 415, 248, 757},
                    {16, 37, 122, 206, 403, 899, 1085},
                    {13, 41, 1, 122, 184, 328, 1630},
                    {9, 13, 69, 67, 279, 518, 64},
                    {7, 9, 92, 147, 198, 477, 689},
                    {15, 49, 47, 54, 307, 404, 1300},
                    {16, 36, 11, 23, 432, 698, 148},
                    {18, 10, 31, 93, 240, 160, 777},
                    {4, 11, 19, 20, 454, 497, 1431},
                    {23, 18, 66, 197, 294, 100, 659},
                    {5, 54, 49, 46, 479, 518, 352},
                    {3, 40, 81, 162, 289, 92, 1177},
                    {29, 27, 96, 101, 373, 464, 836},
                    {11, 35, 38, 76, 104, 592, 1572},
                    {4, 25, 83, 78, 141, 198, 348},
                    {8, 46, 42, 253, 270, 856, 1040},
                    {2, 24, 58, 124, 439, 235, 779},
                    {11, 33, 24, 143, 333, 134, 476},
                    {11, 18, 25, 63, 399, 542, 191},
                    {3, 37, 92, 41, 14, 545, 1393},
                    {15, 35, 38, 214, 277, 777, 1752},
                    {13, 21, 120, 70, 412, 483, 1627},
                },
                // j = 3
                {
                    {0, 0, 0, 0, 0, 0, 0},
                    {13, 44, 35, 162, 312, 285, 1189},
                    {19, 51, 97, 7, 503, 554, 458},
                    {14, 12, 112, 31, 388, 809, 460},
                    {15, 15, 64, 164, 48, 185, 1039},
                    {20, 12, 93, 11, 7, 49, 1000},
                    {17, 4, 99, 237, 185, 101, 1265},
                    {4, 7, 94, 125, 328, 82, 1223},
                    {4, 2, 103, 133, 254, 898, 874},
                    {11, 30, 91, 99, 202, 627, 1292},
                    {17, 53, 3, 105, 285, 154, 1491},
                    {20, 23, 6, 17, 11, 65, 631},
                    {8, 29, 39, 97, 168, 81, 464},
                    {22, 37, 113, 91, 127, 823, 461},
                    {19, 42, 92, 211, 8, 50, 844},
                    {15, 48, 119, 128, 437, 413, 392},
                    {5, 4, 74, 82, 475, 462, 922},
                    {21, 10, 73, 115, 85, 175, 256},
                    {17, 18, 116, 248, 419, 715, 1986},
                    {9, 56, 31, 62, 459, 537, 19},
                    {20, 9, 127, 26, 468, 722, 266},
                    {18, 11, 98, 140, 209, 37, 471},
                    {31, 23, 23, 121, 311, 488, 1166},
                    {13, 8, 38, 12, 211, 179, 1300},
                    {2, 7, 18, 41, 510, 430, 1033},
                    {18, 24, 62, 249, 320, 264, 1606},
                },
            };

            // Table 7-3 and 7-4 in [1]
            size_t mphi(ar4ja_rate_t rate, ar4ja_blocksize_t bk, size_t k, size_t j)
            {
                assert(k <= 26);
                assert(j <= 4);
                int m_index = mlog2(get_m(rate, bk)) - mlog2(M128);
                return PHI_K[j][k - 1][m_index];
            }

            // Section 7.4.2.4 in [1]
            size_t mpi(ar4ja_rate_t rate, ar4ja_blocksize_t bk, size_t k, size_t i)
            {
                int m_log2 = mlog2(get_m(rate, bk));
                int m = 1 << m_log2;
                int j = 4 * i / m;
                // & 0x3 gives mod 4
                int a = (mtheta(k) + j) & 0x3;
                int m_div_4 = 1 << (m_log2 - 2);
                // & (m_div_4 - 1) gives mod M/4
                int b = (mphi(rate, bk, k, j) + i) & (m_div_4 - 1);
                // << (m_log2 - 2) gives * M/4
                return (a << (m_log2 - 2)) + b;
            }

            codings::ldpc::Sparse_matrix make_ar4ja_code(ar4ja_rate_t rate, ar4ja_blocksize_t k, int *M)
            {
                int m = 1 << mlog2(get_m(rate, k));
                int extra_column_blocks = 0;
                if (rate == AR4JA_RATE_1_2)
                    extra_column_blocks = 0;
                else if (rate == AR4JA_RATE_2_3)
                    extra_column_blocks = 2;
                else if (rate == AR4JA_RATE_4_5)
                    extra_column_blocks = 6;
                int extra_columns = m * extra_column_blocks;
                codings::ldpc::Sparse_matrix h(3 * m, extra_columns + 5 * m);

                // fill common part (H_1/2)
                for (int i = 0; i < m; i++)
                {
                    // block(0,2) = I_M
                    h.add_connection(i, extra_columns + 2 * m + i);
                    // block(0,4) = I_M + Pi_1
                    h.add_connection(i, extra_columns + 4 * m + i);
                    mtx_toggle(h, i, extra_columns + 4 * m + mpi(rate, k, 1, i));
                    // block(1,0) = I_M
                    h.add_connection(m + i, extra_columns + i);
                    // block(1,1) = I_M
                    h.add_connection(m + i, extra_columns + m + i);
                    // block(1,3) = I_M
                    h.add_connection(m + i, extra_columns + 3 * m + i);
                    // block(1,4) = Pi_2 + Pi_3 + Pi_4
                    h.add_connection(m + i, extra_columns + 4 * m + mpi(rate, k, 2, i));
                    mtx_toggle(h, m + i, extra_columns + 4 * m + mpi(rate, k, 3, i));
                    mtx_toggle(h, m + i, extra_columns + 4 * m + mpi(rate, k, 4, i));
                    // block(2,0) = I_M
                    h.add_connection(2 * m + i, extra_columns + i);
                    // block(2,1) = Pi_5 + Pi_6
                    h.add_connection(2 * m + i, extra_columns + m + mpi(rate, k, 5, i));
                    mtx_toggle(h, 2 * m + i, extra_columns + m + mpi(rate, k, 6, i));
                    // block(2,3) = Pi_7 + Pi_8
                    h.add_connection(2 * m + i, extra_columns + 3 * m + mpi(rate, k, 7, i));
                    mtx_toggle(h, 2 * m + i, extra_columns + 3 * m + mpi(rate, k, 8, i));
                    // block(2,4) = I_M
                    h.add_connection(2 * m + i, extra_columns + 4 * m + i);
                }

                if (rate != AR4JA_RATE_1_2)
                {
                    // fill specific H_2/3 part
                    if (rate == AR4JA_RATE_2_3)
                        extra_columns = 0;
                    else if (rate == AR4JA_RATE_4_5)
                        extra_columns = 4 * m;

                    for (int i = 0; i < m; i++)
                    {
                        // block(1,0) = Pi_9 + Pi_10 + Pi_11
                        h.add_connection(m + i, extra_columns + mpi(rate, k, 9, i));
                        mtx_toggle(h, m + i, extra_columns + mpi(rate, k, 10, i));
                        mtx_toggle(h, m + i, extra_columns + mpi(rate, k, 11, i));
                        // block(1,1) = I_M
                        h.add_connection(m + i, extra_columns + m + i);
                        // block(2,0) = I_M
                        h.add_connection(2 * m + i, extra_columns + i);
                        // block(2,1) = Pi_12 + Pi_13 + Pi_14
                        h.add_connection(2 * m + i, extra_columns + m + mpi(rate, k, 12, i));
                        mtx_toggle(h, 2 * m + i, extra_columns + m + mpi(rate, k, 13, i));
                        mtx_toggle(h, 2 * m + i, extra_columns + m + mpi(rate, k, 14, i));
                    }
                }

                if (rate == AR4JA_RATE_4_5)
                {
                    // fill specific H_4/5 part
                    for (int i = 0; i < m; i++)
                    {
                        // block(1,0) = Pi_21 + Pi_22 + Pi_23
                        h.add_connection(m + i, mpi(rate, k, 21, i));
                        mtx_toggle(h, m + i, mpi(rate, k, 22, i));
                        mtx_toggle(h, m + i, mpi(rate, k, 23, i));
                        // block(1,1) = I_M
                        h.add_connection(m + i, m + i);
                        // block(1,2) = Pi_15 + Pi_16 + Pi_17
                        h.add_connection(m + i, 2 * m + mpi(rate, k, 15, i));
                        mtx_toggle(h, m + i, 2 * m + mpi(rate, k, 16, i));
                        mtx_toggle(h, m + i, 2 * m + mpi(rate, k, 17, i));
                        // block(1,3) = I_M
                        h.add_connection(m + i, 3 * m + i);
                        // block(2,0) = I_M
                        h.add_connection(2 * m + i, i);
                        // block(2,1) = Pi_24 + Pi_25 + Pi_26
                        h.add_connection(2 * m + i, m + mpi(rate, k, 24, i));
                        mtx_toggle(h, 2 * m + i, m + mpi(rate, k, 25, i));
                        mtx_toggle(h, 2 * m + i, m + mpi(rate, k, 26, i));
                        // block(2,2) = I_M
                        h.add_connection(2 * m + i, 2 * m + i);
                        // block(2,3) = Pi_18 + Pi_19 + Pi_20
                        h.add_connection(2 * m + i, 3 * m + mpi(rate, k, 18, i));
                        mtx_toggle(h, 2 * m + i, 3 * m + mpi(rate, k, 19, i));
                        mtx_toggle(h, 2 * m + i, 3 * m + mpi(rate, k, 20, i));
                    }
                }

                if (M != nullptr)
                    *M = m;

                return h;
            }
        }

        namespace ccsds_78
        {
            /* (8176, 7154) code Parity Check Matrix dimensions */
            const size_t pcm_8176_7154_w = 8176;
            const size_t pcm_8176_7154_h = 1022;
            const size_t pcm_8176_7154_max_deg = 32;
            const size_t pcm_8176_7154_edges = 32704;

            /* (8176, 7154) code subblock characteristics of the parity check matrix */
            const size_t pcm_8176_7154_sb_x = 16;     // subblocks in x dimension
            const size_t pcm_8176_7154_sb_y = 2;      // subblocks in y dimension
            const size_t pcm_8176_7154_sb_size = 511; // subblock size (subblocks are square)
            const size_t pcm_8176_7154_row_deg = 32;  // All rows have the same degree

            /* (8176, 7154) code parity check matrix circulants.
             * Each row of a subblock has exactly 2 non-zero element */
            const uint16_t pcm_8176_7154_circulants[pcm_8176_7154_sb_y][pcm_8176_7154_sb_x][2] = {{{0, 176},
                                                                                                   {12, 239},
                                                                                                   {0, 352},
                                                                                                   {24, 431},
                                                                                                   {0, 392},
                                                                                                   {151, 409},
                                                                                                   {0, 351},
                                                                                                   {9, 359},
                                                                                                   {0, 307},
                                                                                                   {53, 329},
                                                                                                   {0, 207},
                                                                                                   {18, 281},
                                                                                                   {0, 399},
                                                                                                   {202, 457},
                                                                                                   {0, 247},
                                                                                                   {36, 261}},
                                                                                                  {{99, 471},
                                                                                                   {130, 473},
                                                                                                   {198, 435},
                                                                                                   {260, 478},
                                                                                                   {215, 420},
                                                                                                   {282, 481},
                                                                                                   {48, 396},
                                                                                                   {193, 445},
                                                                                                   {273, 430},
                                                                                                   {302, 451},
                                                                                                   {96, 379},
                                                                                                   {191, 386},
                                                                                                   {244, 467},
                                                                                                   {364, 470},
                                                                                                   {51, 382},
                                                                                                   {192, 414}}};

            codings::ldpc::Sparse_matrix make_r78_code()
            {
                /* Subblocks are square */
                const size_t subblock_size = pcm_8176_7154_sb_size;
                const size_t subblocks_x = pcm_8176_7154_sb_x;
                const size_t subblocks_y = pcm_8176_7154_sb_y;

                size_t abs_row;
                uint16_t rel_offset0, rel_offset1;
                uint16_t abs_offset0, abs_offset1;

                codings::ldpc::Sparse_matrix r78(pcm_8176_7154_h, pcm_8176_7154_w);

                for (size_t sblk_y = 0; sblk_y < subblocks_y; sblk_y++)
                {
                    for (size_t row = 0; row < subblock_size; row++)
                    {
                        abs_row = sblk_y * subblock_size + row;

                        for (size_t sblk_x = 0; sblk_x < subblocks_x; sblk_x++)
                        {
                            rel_offset0 = pcm_8176_7154_circulants[sblk_y][sblk_x][0];
                            rel_offset1 = pcm_8176_7154_circulants[sblk_y][sblk_x][1];

                            /* Compute the final absolute offsets of '1's in the
                             * current circulant depending on the current row and
                             * subblock. */
                            rel_offset0 = (rel_offset0 + row) % subblock_size;
                            rel_offset1 = (rel_offset1 + row) % subblock_size;
                            abs_offset0 = subblock_size * sblk_x + rel_offset0;
                            abs_offset1 = subblock_size * sblk_x + rel_offset1;

                            r78.add_connection(abs_row, abs_offset0);
                            r78.add_connection(abs_row, abs_offset1);
                        }
                    }
                }

                return r78;
            }
        }
    }
}