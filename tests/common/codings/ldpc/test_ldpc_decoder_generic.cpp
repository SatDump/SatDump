#include "common/codings/ldpc/ldpc_decoder_generic.h"
#include "common/codings/ldpc/labrador/codes/parity.h"
#include "common/codings/ldpc/labrador/encoder.h"
#include <catch2/catch_test_macros.hpp>
#include <map>
#include <set>
#include <vector>

namespace
{
    using codings::ldpc::LDPCDecoderGeneric;
    using codings::ldpc::Sparse_matrix;

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

    // Hamming (7,4): parity bits at the end so the first n - m columns are the data bits
    Sparse_matrix hamming74()
    {
        const int h[3][7] = {{1, 1, 0, 1, 1, 0, 0},
                             {1, 0, 1, 1, 0, 1, 0},
                             {0, 1, 1, 1, 0, 0, 1}};
        Sparse_matrix m(3, 7);
        for (int r = 0; r < 3; r++)
            for (int c = 0; c < 7; c++)
                if (h[r][c])
                    m.add_connection(r, c);
        return m;
    }

    std::vector<uint8_t> hammingEncode(int data)
    {
        uint8_t d[4] = {uint8_t((data >> 3) & 1), uint8_t((data >> 2) & 1), uint8_t((data >> 1) & 1), uint8_t(data & 1)};
        return {d[0], d[1], d[2], d[3], uint8_t(d[0] ^ d[1] ^ d[3]), uint8_t(d[0] ^ d[2] ^ d[3]), uint8_t(d[1] ^ d[2] ^ d[3])};
    }

    // TC128 parity check matrix from the labrador iterator (duplicate edges cancel)
    Sparse_matrix tc128Matrix()
    {
        std::map<std::pair<uint64_t, uint64_t>, int> count;
        satdump::labrador::parity_iter_t it;
        satdump::labrador::parity_iter_init(&it, satdump::labrador::TC128);
        uint64_t check, var;
        while (satdump::labrador::parity_iter_next(&it, &check, &var))
            count[{check, var}]++;
        Sparse_matrix m(64, 128);
        for (auto &c : count)
            if (c.second % 2)
                m.add_connection(c.first.first, c.first.second);
        return m;
    }

    std::vector<int8_t> toSoft(const std::vector<uint8_t> &bits, int8_t amplitude)
    {
        std::vector<int8_t> soft(bits.size());
        for (size_t i = 0; i < bits.size(); i++)
            soft[i] = bits[i] ? amplitude : -amplitude;
        return soft;
    }
}

TEST_CASE("LDPCDecoderGeneric decodes every Hamming(7,4) codeword and corrects one weak error", "[common][codings][ldpc]")
{
    LDPCDecoderGeneric dec(hamming74());
    CHECK(dec.simd() == 1);

    for (int data = 0; data < 16; data++)
    {
        std::vector<uint8_t> cw = hammingEncode(data);
        std::vector<int8_t> soft = toSoft(cw, 20);
        std::vector<uint8_t> out(7, 0xEE);

        INFO("data " << data);
        CHECK(dec.decode(out.data(), soft.data(), 5) == 0);
        CHECK(out == cw);

        for (int pos = 0; pos < 7; pos++)
        {
            INFO("error at " << pos);
            std::vector<int8_t> bad = soft;
            bad[pos] = bad[pos] > 0 ? -3 : 3;
            CHECK(dec.decode(out.data(), bad.data(), 5) == (pos < 4 ? 1 : 0));
            CHECK(out == cw);
        }
    }
}

TEST_CASE("LDPCDecoderGeneric corrects errors in TC128 codewords and is reusable", "[common][codings][ldpc]")
{
    satdump::labrador::code_params_t c = satdump::labrador::get_code_params(satdump::labrador::TC128);
    LDPCDecoderGeneric dec(tc128Matrix());

    for (uint32_t seed = 1; seed <= 5; seed++)
    {
        INFO("seed " << seed);
        Lcg rng(seed);
        std::vector<uint8_t> packed(16);
        for (auto &b : packed)
            b = rng.next() >> 24;
        satdump::labrador::encode_parity_u8(c, packed.data());
        std::vector<uint8_t> cw(128);
        for (int i = 0; i < 128; i++)
            cw[i] = (packed[i / 8] >> (7 - i % 8)) & 1;

        std::vector<int8_t> soft = toSoft(cw, 30);
        std::set<int> flips;
        while (flips.size() < 3)
            flips.insert(rng.below(128));
        int dataFlips = 0;
        for (int i : flips)
        {
            soft[i] = soft[i] > 0 ? -4 : 4;
            dataFlips += i < 64;
        }

        std::vector<uint8_t> out(128);
        CHECK(dec.decode(out.data(), soft.data(), 20) == dataFlips);
        CHECK(out == cw);
    }
}

TEST_CASE("LDPCDecoderGeneric zero iterations is a plain hard decision", "[common][codings][ldpc]")
{
    LDPCDecoderGeneric dec(hamming74());
    std::vector<int8_t> soft = {5, -5, 0, 127, -128, 1, -1};
    std::vector<uint8_t> out(7);
    // Corrections are counted against in > 0, and 0 decides as 1 (>= 0)
    CHECK(dec.decode(out.data(), soft.data(), 0) == 1);
    CHECK(out == std::vector<uint8_t>({1, 0, 1, 1, 0, 1, 0}));
}
