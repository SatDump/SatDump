#include "common/codings/ldpc/ccsds_ldpc.h"
#include "common/codings/ldpc/make_ccsds.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <set>
#include <stdexcept>
#include <vector>

namespace
{
    using namespace codings::ldpc;

    struct Lcg
    {
        uint32_t state;
        explicit Lcg(uint32_t seed) : state(seed) {}
        uint32_t next()
        {
            state = state * 1664525u + 1013904223u;
            return state;
        }
        int below(int n) { return (next() >> 8) % n; }
    };

    // Systematic GF(2) encoder derived from a parity check matrix by Gauss-Jordan elimination.
    // Pivots are searched from the last column backwards, so parity ends up at the end of the codeword.
    struct Gf2Encoder
    {
        int n = 0;
        int words = 0;
        std::vector<std::vector<uint64_t>> rows; // reduced rows, one per pivot
        std::vector<int> pivots;
        std::vector<bool> isPivot;

        explicit Gf2Encoder(const Sparse_matrix &h)
        {
            n = h.get_n_cols();
            words = (n + 63) / 64;
            std::vector<std::vector<uint64_t>> m(h.get_n_rows(), std::vector<uint64_t>(words, 0));
            for (size_t r = 0; r < h.get_n_rows(); r++)
                for (auto c : h.get_cols_from_row(r))
                    m[r][c / 64] ^= 1ull << (c % 64);

            isPivot.assign(n, false);
            size_t next = 0;
            for (int c = n - 1; c >= 0 && next < m.size(); c--)
            {
                size_t p = next;
                while (p < m.size() && !((m[p][c / 64] >> (c % 64)) & 1))
                    p++;
                if (p == m.size())
                    continue;
                std::swap(m[p], m[next]);
                for (size_t r = 0; r < m.size(); r++)
                    if (r != next && ((m[r][c / 64] >> (c % 64)) & 1))
                        for (int w = 0; w < words; w++)
                            m[r][w] ^= m[next][w];
                pivots.push_back(c);
                isPivot[c] = true;
                next++;
            }
            m.resize(next);
            rows = m;
        }

        // Fills the pivot (parity) positions of `cw` from its free positions
        void encode(std::vector<uint8_t> &cw) const
        {
            for (size_t i = 0; i < pivots.size(); i++)
            {
                uint8_t v = 0;
                for (int c = 0; c < n; c++)
                    if (!isPivot[c] && ((rows[i][c / 64] >> (c % 64)) & 1))
                        v ^= cw[c];
                cw[pivots[i]] = v;
            }
        }
    };

    bool satisfiesParity(const Sparse_matrix &h, const std::vector<uint8_t> &cw)
    {
        for (size_t r = 0; r < h.get_n_rows(); r++)
        {
            uint8_t s = 0;
            for (auto c : h.get_cols_from_row(r))
                s ^= cw[c];
            if (s)
                return false;
        }
        return true;
    }

    // Random codeword with `zeroPrefix` leading zero (shortened) bits
    std::vector<uint8_t> randomCodeword(const Sparse_matrix &h, const Gf2Encoder &enc, uint32_t seed, int zeroPrefix = 0)
    {
        Lcg rng(seed);
        std::vector<uint8_t> cw(h.get_n_cols(), 0);
        for (size_t i = zeroPrefix; i < cw.size(); i++)
            if (!enc.isPivot[i])
                cw[i] = rng.next() >> 31;
        enc.encode(cw);
        return cw;
    }

    struct LdpcCase
    {
        ldpc_rate_t rate;
        int block;
        const char *name;
    };

    Sparse_matrix matrixFor(const LdpcCase &c)
    {
        if (c.rate == RATE_7_8)
            return ccsds_78::make_r78_code();
        ccsds_ar4ja::ar4ja_rate_t r = c.rate == RATE_1_2 ? ccsds_ar4ja::AR4JA_RATE_1_2 : c.rate == RATE_2_3 ? ccsds_ar4ja::AR4JA_RATE_2_3 : ccsds_ar4ja::AR4JA_RATE_4_5;
        return ccsds_ar4ja::make_ar4ja_code(r, ccsds_ar4ja::AR4JA_BLOCK_1024);
    }

    // Positive soft = 1. Transmitted bits of the full codeword: AR4JA drops the last M, 7/8 drops
    // the 18 leading fill bits and appends 2 zero bits.
    std::vector<int8_t> transmit(const LdpcCase &c, const std::vector<uint8_t> &cw, int frameSize, int8_t amplitude)
    {
        std::vector<int8_t> soft(frameSize, -amplitude);
        int offset = c.rate == RATE_7_8 ? 18 : 0;
        int count = c.rate == RATE_7_8 ? 8158 : frameSize;
        for (int i = 0; i < count; i++)
            soft[i] = cw[offset + i] ? amplitude : -amplitude;
        return soft;
    }
}

TEST_CASE("ldpc_rate_from_string parses CCSDS LDPC rates", "[common][codings][ldpc]")
{
    CHECK(ldpc_rate_from_string("1/2") == RATE_1_2);
    CHECK(ldpc_rate_from_string("2/3") == RATE_2_3);
    CHECK(ldpc_rate_from_string("4/5") == RATE_4_5);
    CHECK(ldpc_rate_from_string("7/8") == RATE_7_8);
    CHECK_THROWS_AS(ldpc_rate_from_string("3/4"), std::runtime_error);
    CHECK_THROWS_AS(ldpc_rate_from_string(""), std::runtime_error);
}

TEST_CASE("CCSDSLDPC rejects unsupported AR4JA block sizes", "[common][codings][ldpc]")
{
    CHECK_THROWS_AS(CCSDSLDPC(RATE_1_2, 2048), std::runtime_error);
    CHECK_THROWS_AS(CCSDSLDPC(RATE_4_5, 0), std::runtime_error);
}

TEST_CASE("CCSDSLDPC frame and codeword lengths", "[common][codings][ldpc]")
{
    // Large block sizes are left out: the generic decoder's constructor scans the dense H
    CCSDSLDPC r12(RATE_1_2, 1024);
    CHECK(r12.frame_length() == 2048);
    CHECK(r12.codeword_length() == 2560);
    CHECK(r12.data_length() == 1024);

    CCSDSLDPC r23(RATE_2_3, 1024);
    CHECK(r23.frame_length() == 1536);
    CHECK(r23.codeword_length() == 1792);

    CCSDSLDPC r45(RATE_4_5, 1024);
    CHECK(r45.frame_length() == 1280);
    CHECK(r45.codeword_length() == 1408);

    CCSDSLDPC r78(RATE_7_8, 0);
    CHECK(r78.frame_length() == 8160);
    CHECK(r78.codeword_length() == 8176);
    CHECK(r78.data_length() == 7136);

    // No SIMD plugins are loaded in the test binary, so the generic decoder is used
    CHECK(r12.simd() == 1);
}

// BUG: data_length() is computed as n_rows - M = 2M, which is only k for rate 1/2. For AR4JA rate
// 2/3 and 4/5 the information length is n_cols - n_rows = 1024 here (ccsds_ldpc.cpp:54).
TEST_CASE("CCSDSLDPC data_length is the information block size for every AR4JA rate", "[common][codings][ldpc][!shouldfail]")
{
    CCSDSLDPC r23(RATE_2_3, 1024);
    CHECK(r23.data_length() == 1024);
    CCSDSLDPC r45(RATE_4_5, 1024);
    CHECK(r45.data_length() == 1024);
}

TEST_CASE("CCSDSLDPC decodes clean and corrupted codewords", "[common][codings][ldpc]")
{
    auto tc = GENERATE(LdpcCase{RATE_1_2, 1024, "1/2"}, LdpcCase{RATE_2_3, 1024, "2/3"}, LdpcCase{RATE_4_5, 1024, "4/5"}, LdpcCase{RATE_7_8, 0, "7/8"});
    INFO("rate " << tc.name);

    Sparse_matrix h = matrixFor(tc);
    Gf2Encoder enc(h);
    int zeroPrefix = tc.rate == RATE_7_8 ? 18 : 0;
    for (int p : enc.pivots)
        REQUIRE(p >= zeroPrefix); // fill bits must be information positions
    std::vector<uint8_t> cw = randomCodeword(h, enc, 1234 + tc.rate, zeroPrefix);
    REQUIRE(satisfiesParity(h, cw));

    CCSDSLDPC ldpc(tc.rate, tc.block);
    const int frameSize = ldpc.frame_length();
    // Information positions the decoder counts corrections over, in frame coordinates
    const int infoBits = int(h.get_n_cols() - h.get_n_rows()) - zeroPrefix;
    const int transmitted = tc.rate == RATE_7_8 ? 8158 : frameSize;

    SECTION("clean")
    {
        std::vector<int8_t> soft = transmit(tc, cw, frameSize, 40);
        std::vector<uint8_t> out(frameSize, 0xEE);
        CHECK(ldpc.decode(soft.data(), out.data(), 10) == 0);
        for (int i = 0; i < transmitted; i++)
            REQUIRE(out[i] == cw[zeroPrefix + i]);
    }

    SECTION("with weak sign errors")
    {
        std::vector<int8_t> soft = transmit(tc, cw, frameSize, 40);
        // Flip ~1% of the transmitted bits to the wrong side with low confidence
        Lcg rng(77 + tc.rate);
        std::set<int> flipped;
        int target = transmitted / 100;
        while ((int)flipped.size() < target)
            flipped.insert(rng.below(transmitted));
        int infoFlips = 0;
        for (int i : flipped)
        {
            soft[i] = soft[i] > 0 ? -8 : 8;
            infoFlips += i < infoBits;
        }

        std::vector<uint8_t> out(frameSize, 0xEE);
        int corrected = ldpc.decode(soft.data(), out.data(), 20);
        int errors = 0;
        for (int i = 0; i < transmitted; i++)
            errors += out[i] != cw[zeroPrefix + i];
        CHECK(errors == 0);
        // The returned count is the number of information bits whose hard decision changed
        CHECK(corrected == infoFlips);
    }
}

TEST_CASE("CCSDSLDPC decode with zero iterations returns hard decisions", "[common][codings][ldpc]")
{
    CCSDSLDPC ldpc(RATE_1_2, 1024);
    std::vector<int8_t> soft(ldpc.frame_length());
    Lcg rng(5);
    for (auto &s : soft)
        s = (rng.next() >> 31) ? 20 : -20;
    std::vector<uint8_t> out(ldpc.frame_length(), 0xEE);
    CHECK(ldpc.decode(soft.data(), out.data(), 0) == 0);
    for (size_t i = 0; i < soft.size(); i++)
        REQUIRE(out[i] == (soft[i] > 0 ? 1 : 0));
}
