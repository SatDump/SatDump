#include "common/codings/ldpc/make_ccsds.h"
#include "common/codings/ldpc/labrador/codes/parity.h"
#include <catch2/catch_test_macros.hpp>
#include <map>
#include <set>
#include <utility>

namespace
{
    using namespace codings::ldpc;

    struct Ar4jaCase
    {
        ccsds_ar4ja::ar4ja_rate_t rate;
        ccsds_ar4ja::ar4ja_blocksize_t block;
        int k;
        int m;          // circulant size from CCSDS 131.0-B table 7-3
        int colBlocks;  // 5 for r=1/2, 7 for 2/3, 11 for 4/5
    };

    const Ar4jaCase AR4JA_CASES[] = {
        {ccsds_ar4ja::AR4JA_RATE_1_2, ccsds_ar4ja::AR4JA_BLOCK_1024, 1024, 512, 5},
        {ccsds_ar4ja::AR4JA_RATE_2_3, ccsds_ar4ja::AR4JA_BLOCK_1024, 1024, 256, 7},
        {ccsds_ar4ja::AR4JA_RATE_4_5, ccsds_ar4ja::AR4JA_BLOCK_1024, 1024, 128, 11},
        {ccsds_ar4ja::AR4JA_RATE_1_2, ccsds_ar4ja::AR4JA_BLOCK_4096, 4096, 2048, 5},
        {ccsds_ar4ja::AR4JA_RATE_2_3, ccsds_ar4ja::AR4JA_BLOCK_4096, 4096, 1024, 7},
        {ccsds_ar4ja::AR4JA_RATE_4_5, ccsds_ar4ja::AR4JA_BLOCK_4096, 4096, 512, 11},
        {ccsds_ar4ja::AR4JA_RATE_1_2, ccsds_ar4ja::AR4JA_BLOCK_16384, 16384, 8192, 5},
        {ccsds_ar4ja::AR4JA_RATE_2_3, ccsds_ar4ja::AR4JA_BLOCK_16384, 16384, 4096, 7},
        {ccsds_ar4ja::AR4JA_RATE_4_5, ccsds_ar4ja::AR4JA_BLOCK_16384, 16384, 2048, 11},
    };

    // Set of H positions from the labrador parity iterator, summing duplicate edges modulo 2
    std::set<std::pair<uint64_t, uint64_t>> labradorEdges(satdump::labrador::ldpc_code_t code)
    {
        std::map<std::pair<uint64_t, uint64_t>, int> count;
        satdump::labrador::parity_iter_t it;
        satdump::labrador::parity_iter_init(&it, code);
        uint64_t check, var;
        while (satdump::labrador::parity_iter_next(&it, &check, &var))
            count[{check, var}]++;
        std::set<std::pair<uint64_t, uint64_t>> edges;
        for (auto &c : count)
            if (c.second % 2)
                edges.insert(c.first);
        return edges;
    }

    std::set<std::pair<uint64_t, uint64_t>> matrixEdges(const Sparse_matrix &h)
    {
        std::set<std::pair<uint64_t, uint64_t>> edges;
        for (size_t r = 0; r < h.get_n_rows(); r++)
            for (auto c : h.get_cols_from_row(r))
                edges.insert({r, c});
        return edges;
    }
}

TEST_CASE("make_ar4ja_code dimensions and circulant size for every rate and block size", "[common][codings][ldpc]")
{
    for (auto &tc : AR4JA_CASES)
    {
        INFO("k = " << tc.k << ", rate enum " << int(tc.rate));
        int m = -1;
        Sparse_matrix h = ccsds_ar4ja::make_ar4ja_code(tc.rate, tc.block, &m);
        CHECK(m == tc.m);
        CHECK(h.get_n_rows() == size_t(3 * tc.m));
        CHECK(h.get_n_cols() == size_t(tc.colBlocks * tc.m));
        // k = n_cols - n_rows information bits, last M columns are the punctured ones
        CHECK(int(h.get_n_cols() - h.get_n_rows()) == tc.k);
        CHECK(h.get_n_connections() > 0);

        // Row and column index lists are consistent
        size_t fromCols = 0;
        for (size_t c = 0; c < h.get_n_cols(); c++)
            fromCols += h.get_rows_from_col(c).size();
        CHECK(fromCols == h.get_n_connections());
    }
}

TEST_CASE("make_ar4ja_code M output pointer is optional", "[common][codings][ldpc]")
{
    Sparse_matrix h = ccsds_ar4ja::make_ar4ja_code(ccsds_ar4ja::AR4JA_RATE_1_2, ccsds_ar4ja::AR4JA_BLOCK_1024);
    CHECK(h.get_n_rows() == 1536);
}

TEST_CASE("make_ar4ja_code matches the labrador parity check iterator for TM codes", "[common][codings][ldpc]")
{
    struct
    {
        satdump::labrador::ldpc_code_t code;
        ccsds_ar4ja::ar4ja_rate_t rate;
        ccsds_ar4ja::ar4ja_blocksize_t block;
    } cases[] = {
        {satdump::labrador::TM2048, ccsds_ar4ja::AR4JA_RATE_1_2, ccsds_ar4ja::AR4JA_BLOCK_1024},
        {satdump::labrador::TM1536, ccsds_ar4ja::AR4JA_RATE_2_3, ccsds_ar4ja::AR4JA_BLOCK_1024},
        {satdump::labrador::TM1280, ccsds_ar4ja::AR4JA_RATE_4_5, ccsds_ar4ja::AR4JA_BLOCK_1024},
        {satdump::labrador::TM8192, ccsds_ar4ja::AR4JA_RATE_1_2, ccsds_ar4ja::AR4JA_BLOCK_4096},
        {satdump::labrador::TM6144, ccsds_ar4ja::AR4JA_RATE_2_3, ccsds_ar4ja::AR4JA_BLOCK_4096},
        {satdump::labrador::TM5120, ccsds_ar4ja::AR4JA_RATE_4_5, ccsds_ar4ja::AR4JA_BLOCK_4096},
    };

    for (auto &tc : cases)
    {
        INFO("labrador code " << int(tc.code));
        Sparse_matrix h = ccsds_ar4ja::make_ar4ja_code(tc.rate, tc.block);
        CHECK(matrixEdges(h) == labradorEdges(tc.code));
    }
}

TEST_CASE("make_r78_code builds the CCSDS (8176,7154) quasi-cyclic matrix", "[common][codings][ldpc]")
{
    Sparse_matrix h = ccsds_78::make_r78_code();
    REQUIRE(h.get_n_rows() == 1022);
    REQUIRE(h.get_n_cols() == 8176);

    // 2 x 16 circulants of size 511, each of row and column weight 2
    CHECK(h.get_n_connections() == 1022 * 32);
    CHECK(h.get_rows_max_degree() == 32);
    CHECK(h.get_cols_max_degree() == 4);
    for (size_t r = 0; r < h.get_n_rows(); r++)
        REQUIRE(h.get_cols_from_row(r).size() == 32);
    for (size_t c = 0; c < h.get_n_cols(); c++)
        REQUIRE(h.get_rows_from_col(c).size() == 4);

    // Quasi-cyclic: shifting a row by one inside each circulant gives the next row
    for (size_t r = 0; r + 1 < 511; r += 50)
    {
        std::set<uint32_t> next;
        for (auto c : h.get_cols_from_row(r))
            next.insert((c / 511) * 511 + (c % 511 + 1) % 511);
        auto actual = h.get_cols_from_row(r + 1);
        CHECK(next == std::set<uint32_t>(actual.begin(), actual.end()));
    }
}
