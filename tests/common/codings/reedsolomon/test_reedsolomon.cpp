#include "common/codings/reedsolomon/reedsolomon.h"
#include "helpers/random.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstring>
#include <set>
#include <vector>

// The dual basis tables are data symbols of satdump_core that no header declares, so they are
// not exported from the Windows DLL: the tests that use them are skipped with MSVC
#ifndef _MSC_VER
namespace reedsolomon
{
    // Defined (non-static) in reedsolomon.cpp but not declared in the header
    extern unsigned char ToDualBasis[256];
    extern unsigned char FromDualBasis[256];
} // namespace reedsolomon
#endif

namespace
{
    using test_helpers::Lcg;
    using test_helpers::randomBytes;

#ifndef _MSC_VER
    // Only used by the dual basis tests (see above)

    // Independent GF(2^8) arithmetic with the CCSDS field polynomial x^8+x^7+x^2+x+1 (0x187)
    struct GF256
    {
        uint8_t exp[512];
        int log[256];
        GF256()
        {
            int x = 1;
            for (int i = 0; i < 255; i++)
            {
                exp[i] = x;
                log[x] = i;
                x <<= 1;
                if (x & 0x100)
                    x ^= 0x187;
            }
            for (int i = 255; i < 512; i++)
                exp[i] = exp[i - 255];
            log[0] = -1;
        }
        uint8_t mul(uint8_t a, uint8_t b) const
        {
            if (a == 0 || b == 0)
                return 0;
            return exp[log[a] + log[b]];
        }
        uint8_t powAlpha(int e) const { return exp[((e % 255) + 255) % 255]; }
    };

    // CCSDS 131.0-B: roots are alpha^(11*j), j = 128-E .. 127+E, first symbol is the highest degree.
    // Takes a conventional-basis codeword.
    bool syndromesAreZero(const uint8_t *cw, int parity)
    {
        static const GF256 gf;
        int first = 128 - parity / 2;
        for (int j = first; j < first + parity; j++)
        {
            uint8_t root = gf.powAlpha(11 * j);
            uint8_t s = 0;
            for (int i = 0; i < 255; i++) // Horner
                s = gf.mul(s, root) ^ cw[i];
            if (s != 0)
                return false;
        }
        return true;
    }

    // Conventional -> dual basis map built from the CCSDS transformation matrix (as in Phil Karn's gen_ccsds_tal.c)
    uint8_t referenceToDual(uint8_t v)
    {
        static const uint8_t tal[8] = {0x8d, 0xef, 0xec, 0x86, 0xfa, 0x99, 0xaf, 0x7b};
        uint8_t out = 0;
        for (int k = 0; k < 8; k++)
            if (v & (1 << k))
                out ^= tal[7 - k];
        return out;
    }
#endif

    int dataLength(reedsolomon::RS_TYPE type) { return type == reedsolomon::RS223 ? 223 : 239; }

    // Corrupt `count` distinct symbols in [lo, hi) with non-zero error values
    std::set<int> corrupt(uint8_t *cw, int count, uint32_t seed, int lo = 0, int hi = 255)
    {
        Lcg rng(seed);
        std::set<int> pos;
        while ((int)pos.size() < count)
            pos.insert(lo + rng.below(hi - lo));
        for (int p : pos)
        {
            uint8_t e = 0;
            while (e == 0)
                e = rng.byte();
            cw[p] ^= e;
        }
        return pos;
    }

    std::vector<uint8_t> makeCodeword(reedsolomon::RS_TYPE type, bool ccsds, uint32_t seed)
    {
        std::vector<uint8_t> cw = randomBytes(255, seed);
        std::memset(&cw[dataLength(type)], 0, 255 - dataLength(type));
        reedsolomon::ReedSolomon rs(type);
        rs.encode(cw.data(), ccsds);
        return cw;
    }
} // namespace

#ifndef _MSC_VER // needs the dual basis tables (see above)
TEST_CASE("ReedSolomon dual basis tables match the CCSDS transformation and are inverses", "[common][codings][reedsolomon]")
{
    std::set<int> seen;
    for (int i = 0; i < 256; i++)
    {
        INFO("value = " << i);
        CHECK(reedsolomon::ToDualBasis[i] == referenceToDual(i));
        CHECK(reedsolomon::FromDualBasis[reedsolomon::ToDualBasis[i]] == i);
        seen.insert(reedsolomon::ToDualBasis[i]);
    }
    CHECK(seen.size() == 256);
}

TEST_CASE("ReedSolomon::encode produces valid CCSDS codewords", "[common][codings][reedsolomon]")
{
    auto type = GENERATE(reedsolomon::RS223, reedsolomon::RS239);
    bool ccsds = GENERATE(false, true);
    int k = dataLength(type);
    INFO("k = " << k << ", dual basis = " << ccsds);

    for (uint32_t seed = 1; seed <= 5; seed++)
    {
        std::vector<uint8_t> msg = randomBytes(255, seed);
        std::vector<uint8_t> cw = makeCodeword(type, ccsds, seed);

        // Systematic: message symbols untouched
        CHECK(std::equal(msg.begin(), msg.begin() + k, cw.begin()));

        std::vector<uint8_t> conv = cw;
        if (ccsds)
            for (auto &b : conv)
                b = reedsolomon::FromDualBasis[b];
        CHECK(syndromesAreZero(conv.data(), 255 - k));
    }
}
#endif

TEST_CASE("ReedSolomon::encode of all-zero message gives all-zero codeword", "[common][codings][reedsolomon]")
{
    auto type = GENERATE(reedsolomon::RS223, reedsolomon::RS239);
    bool ccsds = GENERATE(false, true);
    std::vector<uint8_t> cw(255, 0);
    reedsolomon::ReedSolomon rs(type);
    rs.encode(cw.data(), ccsds);
    CHECK(cw == std::vector<uint8_t>(255, 0));
}

TEST_CASE("ReedSolomon::decode of a clean codeword reports zero errors", "[common][codings][reedsolomon]")
{
    auto type = GENERATE(reedsolomon::RS223, reedsolomon::RS239);
    bool ccsds = GENERATE(false, true);
    std::vector<uint8_t> cw = makeCodeword(type, ccsds, 42);
    std::vector<uint8_t> copy = cw;

    reedsolomon::ReedSolomon rs(type);
    CHECK(rs.decode(copy.data(), ccsds) == 0);
    CHECK(copy == cw);
}

TEST_CASE("ReedSolomon::decode corrects up to t symbol errors in the data part", "[common][codings][reedsolomon]")
{
    auto type = GENERATE(reedsolomon::RS223, reedsolomon::RS239);
    bool ccsds = GENERATE(false, true);
    int k = dataLength(type);
    int t = (255 - k) / 2;
    reedsolomon::ReedSolomon rs(type);

    for (int nerr = 1; nerr <= t; nerr++)
    {
        INFO("k = " << k << ", dual basis = " << ccsds << ", errors = " << nerr);
        std::vector<uint8_t> cw = makeCodeword(type, ccsds, 100 + nerr);
        std::vector<uint8_t> rx = cw;
        corrupt(rx.data(), nerr, 7 * nerr, 0, k);

        CHECK(rs.decode(rx.data(), ccsds) == nerr);
        CHECK(std::equal(cw.begin(), cw.begin() + k, rx.begin()));
    }
}

TEST_CASE("ReedSolomon::decode corrects mixed data/parity errors, counting only data symbols", "[common][codings][reedsolomon]")
{
    auto type = GENERATE(reedsolomon::RS223, reedsolomon::RS239);
    int k = dataLength(type);
    int t = (255 - k) / 2;
    reedsolomon::ReedSolomon rs(type);

    std::vector<uint8_t> cw = makeCodeword(type, true, 9);
    std::vector<uint8_t> rx = cw;
    int dataErrors = t / 2;
    corrupt(rx.data(), dataErrors, 11, 0, k);
    corrupt(rx.data(), t - dataErrors, 12, k, 255);

    // The returned count only covers the data symbols, and the parity symbols are left as received
    CHECK(rs.decode(rx.data(), true) == dataErrors);
    CHECK(std::equal(cw.begin(), cw.begin() + k, rx.begin()));
}

TEST_CASE("ReedSolomon::decode reports failure beyond capability and leaves data untouched", "[common][codings][reedsolomon]")
{
    auto type = GENERATE(reedsolomon::RS223, reedsolomon::RS239);
    bool ccsds = GENERATE(false, true);
    reedsolomon::ReedSolomon rs(type);

    for (uint32_t seed = 1; seed <= 5; seed++)
    {
        INFO("seed = " << seed);
        std::vector<uint8_t> rx = makeCodeword(type, ccsds, seed);
        corrupt(rx.data(), 60, seed * 13);
        std::vector<uint8_t> before = rx;

        CHECK(rs.decode(rx.data(), ccsds) == -1);
        CHECK(rx == before);
    }
}

TEST_CASE("ReedSolomon deinterleave/interleave use symbol-interleaved layout", "[common][codings][reedsolomon]")
{
    const int depth = 4;
    reedsolomon::ReedSolomon rs(reedsolomon::RS223);
    std::vector<uint8_t> frame = randomBytes(255 * depth, 5);
    std::vector<uint8_t> rebuilt(255 * depth, 0);

    for (int pos = 0; pos < depth; pos++)
    {
        uint8_t cw[255];
        rs.deinterleave(frame.data(), cw, pos, depth);
        for (int i = 0; i < 255; i++)
            REQUIRE(cw[i] == frame[i * depth + pos]);
        rs.interleave(cw, rebuilt.data(), pos, depth);
    }
    CHECK(rebuilt == frame);
}

#ifndef _MSC_VER // needs the dual basis tables (see above)
TEST_CASE("ReedSolomon interleaved encode/decode round-trip with per-codeword error counts", "[common][codings][reedsolomon]")
{
    auto type = GENERATE(reedsolomon::RS223, reedsolomon::RS239);
    int depth = GENERATE(1, 2, 4, 5, 8);
    bool ccsds = GENERATE(false, true);
    int k = dataLength(type);
    int t = (255 - k) / 2;
    INFO("k = " << k << ", depth = " << depth << ", dual basis = " << ccsds);

    reedsolomon::ReedSolomon rs(type);
    std::vector<uint8_t> frame = randomBytes(255 * depth, depth);
    rs.encode_interlaved(frame.data(), ccsds, depth);

    // Data part of the interleaved frame (first k*depth bytes) is unchanged by encoding
    std::vector<uint8_t> msg = randomBytes(255 * depth, depth);
    CHECK(std::equal(msg.begin(), msg.begin() + k * depth, frame.begin()));

    // Each de-interleaved codeword is valid
    for (int pos = 0; pos < depth; pos++)
    {
        uint8_t cw[255];
        rs.deinterleave(frame.data(), cw, pos, depth);
        if (ccsds)
            for (auto &b : cw)
                b = reedsolomon::FromDualBasis[b];
        CHECK(syndromesAreZero(cw, 255 - k));
    }

    // Inject a different number of errors into each codeword's data symbols
    std::vector<uint8_t> rx = frame;
    std::vector<int> expected(depth);
    Lcg rng(depth * 31 + ccsds);
    for (int pos = 0; pos < depth; pos++)
    {
        expected[pos] = pos % (t + 1);
        std::set<int> syms;
        while ((int)syms.size() < expected[pos])
            syms.insert(rng.below(k));
        for (int s : syms)
            rx[s * depth + pos] ^= 1 + rng.below(255);
    }

    // Make the last codeword uncorrectable (random pattern: regular constant patterns can land
    // within t of another codeword and get legitimately miscorrected)
    if (depth > 1)
    {
        uint8_t cw[255];
        rs.deinterleave(rx.data(), cw, depth - 1, depth);
        corrupt(cw, 60, depth);
        rs.interleave(cw, rx.data(), depth - 1, depth);
        expected[depth - 1] = -1;
    }

    std::vector<int> errors(depth, 1234);
    rs.decode_interlaved(rx.data(), ccsds, depth, errors.data());
    CHECK(errors == expected);

    size_t mismatches = 0, first = 0;
    for (int pos = 0; pos < (depth > 1 ? depth - 1 : depth); pos++)
        for (int s = 0; s < k; s++)
            if (!(rx[s * depth + pos] == frame[s * depth + pos]) && mismatches++ == 0)
                first = s * depth + pos;
    INFO("first mismatch at byte " << first);
    REQUIRE(mismatches == 0);
}
#endif

TEST_CASE("ReedSolomon shortened (fill bytes) decode corrects errors at the start of the data", "[common][codings][reedsolomon]")
{
    auto type = GENERATE(reedsolomon::RS223, reedsolomon::RS239);
    int fill = GENERATE(1, 33, 100);
    bool ccsds = GENERATE(false, true);
    int k = dataLength(type);
    int t = (255 - k) / 2;
    INFO("k = " << k << ", fill = " << fill << ", dual basis = " << ccsds);

    // Shortened code: virtual leading zeros are not transmitted
    std::vector<uint8_t> full = randomBytes(255, fill);
    std::memset(full.data(), 0, fill);
    std::memset(&full[k], 0, 255 - k);
    reedsolomon::ReedSolomon(type).encode(full.data(), ccsds);
    std::vector<uint8_t> tx(full.begin() + fill, full.end()); // 255 - fill symbols

    reedsolomon::ReedSolomon rs(type, fill);

    std::vector<uint8_t> rx(255, 0xEE);
    std::copy(tx.begin(), tx.end(), rx.begin());
    CHECK(rs.decode(rx.data(), ccsds) == 0);
    CHECK(std::equal(tx.begin(), tx.end(), rx.begin()));

    // Errors confined to the first k - 2*fill transmitted data symbols are fully corrected
    int region = k - 2 * fill;
    std::copy(tx.begin(), tx.end(), rx.begin());
    corrupt(rx.data(), t, fill * 3, 0, region);
    CHECK(rs.decode(rx.data(), ccsds) == t);
    CHECK(std::equal(tx.begin(), tx.begin() + (k - fill), rx.begin()));
}

// Bug: reedsolomon.cpp copies back only (d_coded_bits - fill_bytes) corrected bytes starting at offset 0
// of the depunctured buffer (which begins with fill_bytes virtual zeros), so the last fill_bytes data
// symbols of a shortened codeword are returned uncorrected even though the error count includes them
// (reedsolomon.cpp:104).
TEST_CASE("ReedSolomon shortened decode corrects errors in the last data symbols", "[common][codings][reedsolomon][!shouldfail]")
{
    const int fill = 33;
    const int k = 223;
    std::vector<uint8_t> full = randomBytes(255, 77);
    std::memset(full.data(), 0, fill);
    std::memset(&full[k], 0, 255 - k);
    reedsolomon::ReedSolomon(reedsolomon::RS223).encode(full.data(), false);
    std::vector<uint8_t> tx(full.begin() + fill, full.end());

    reedsolomon::ReedSolomon rs(reedsolomon::RS223, fill);
    std::vector<uint8_t> rx(255, 0);
    std::copy(tx.begin(), tx.end(), rx.begin());
    int lastData = k - fill - 1; // last transmitted data symbol
    rx[lastData] ^= 0xFF;

    CHECK(rs.decode(rx.data(), false) == 1);
    CHECK(rx[lastData] == tx[lastData]);
}

// Bug: encode()/encode_interlaved() ignore fill_bytes: the de-interleaved 255 - fill symbols are
// encoded as if they were an unshortened codeword (data in the first 223 positions) and only the first
// 255 - fill symbols are interleaved back, so the parity is truncated and the result cannot be decoded
// by decode_interlaved() with the same fill (reedsolomon.cpp:118-142).
TEST_CASE("ReedSolomon shortened interleaved encode/decode round-trip", "[common][codings][reedsolomon][!shouldfail]")
{
    const int fill = 20, depth = 2, k = 223;
    reedsolomon::ReedSolomon rs(reedsolomon::RS223, fill);
    std::vector<uint8_t> frame = randomBytes((255 - fill) * depth, 3);
    std::fill(frame.begin() + (k - fill) * depth, frame.end(), 0);
    rs.encode_interlaved(frame.data(), false, depth);

    std::vector<uint8_t> rx = frame;
    std::vector<int> errors(depth, 1234);
    rs.decode_interlaved(rx.data(), false, depth, errors.data());
    CHECK(errors == std::vector<int>(depth, 0));
}

TEST_CASE("ReedSolomon decode_interlaved with fill bytes corrects shortened codewords", "[common][codings][reedsolomon]")
{
    const int fill = 50, depth = 3, k = 223;
    const int len = 255 - fill;
    reedsolomon::ReedSolomon full(reedsolomon::RS223);
    std::vector<uint8_t> frame(len * depth);
    for (int pos = 0; pos < depth; pos++)
    {
        // Shortened codeword = full codeword with `fill` leading zeros removed
        std::vector<uint8_t> cw = randomBytes(255, 60 + pos);
        std::memset(cw.data(), 0, fill);
        std::memset(&cw[k], 0, 255 - k);
        full.encode(cw.data(), true);
        for (int i = 0; i < len; i++)
            frame[i * depth + pos] = cw[fill + i];
    }

    std::vector<uint8_t> rx = frame;
    Lcg rng(8);
    for (int pos = 0; pos < depth; pos++)
        for (int e = 0; e < 4 + pos; e++) // distinct symbols in the safely-corrected data region
            rx[(e * 20 + pos) * depth + pos] ^= 1 + rng.below(255);

    reedsolomon::ReedSolomon rs(reedsolomon::RS223, fill);
    std::vector<int> errors(depth, 1234);
    rs.decode_interlaved(rx.data(), true, depth, errors.data());
    CHECK(errors == std::vector<int>({4, 5, 6}));
    for (int i = 0; i < (k - fill) * depth; i++)
        REQUIRE(rx[i] == frame[i]);
}
