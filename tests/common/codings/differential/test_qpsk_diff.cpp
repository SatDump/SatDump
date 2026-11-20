#include "common/codings/differential/qpsk_diff.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

namespace
{
    // Symbols are 2-bit values: bit 1 = I (X) sign bit, bit 0 = Q (Y) sign bit.
    // A 90 degree constellation rotation (I, Q) -> (-Q, I) maps (X, Y) -> (!Y, X).
    uint8_t rotate90(uint8_t s)
    {
        uint8_t x = (s >> 1) & 1, y = s & 1;
        return (!y) << 1 | x;
    }

    // Decodes the transition prev -> cur with a fresh decoder. The decoder only
    // starts producing output from its third input, so one dummy symbol is prepended.
    uint8_t decodePair(uint8_t prev, uint8_t cur, bool swap = false)
    {
        diff::QPSKDiff dec;
        dec.swap = swap;
        uint8_t in[3] = {0, prev, cur};
        uint8_t out[2] = {0xFF, 0xFF};
        dec.work(in, 3, out);
        return out[0] << 1 | out[1];
    }

    std::vector<uint8_t> makeSymbols(int count, uint32_t seed)
    {
        std::vector<uint8_t> s(count);
        for (auto &v : s)
        {
            seed = seed * 1664525u + 1013904223u;
            v = seed >> 30;
        }
        return s;
    }

    // Differential encoder built by inverting the decoder's transition table
    std::vector<uint8_t> encode(const std::vector<uint8_t> &data, uint8_t start)
    {
        uint8_t table[4][4]; // table[prev][data] = symbol
        for (int p = 0; p < 4; p++)
            for (int s = 0; s < 4; s++)
                table[p][decodePair(p, s, false)] = s;

        std::vector<uint8_t> out;
        uint8_t prev = start;
        out.push_back(prev);
        for (uint8_t d : data)
            out.push_back(prev = table[prev][d]);
        return out;
    }
} // namespace

TEST_CASE("QPSKDiff transitions from any symbol cover all four outputs", "[common][codings][differential]")
{
    // A valid differential decoder must be invertible for every previous symbol
    for (int prev = 0; prev < 4; prev++)
    {
        bool seen[4] = {false, false, false, false};
        for (int cur = 0; cur < 4; cur++)
        {
            uint8_t d = decodePair(prev, cur);
            REQUIRE(d < 4);
            seen[d] = true;
        }
        INFO("previous symbol " << prev);
        CHECK((seen[0] && seen[1] && seen[2] && seen[3]));
    }
}

TEST_CASE("QPSKDiff no phase change decodes to 0", "[common][codings][differential]")
{
    for (int s = 0; s < 4; s++)
        CHECK(decodePair(s, s) == 0);
}

TEST_CASE("QPSKDiff decoding is invariant to 90 degree constellation rotations", "[common][codings][differential]")
{
    for (int prev = 0; prev < 4; prev++)
    {
        for (int cur = 0; cur < 4; cur++)
        {
            uint8_t expected = decodePair(prev, cur);
            uint8_t p = prev, c = cur;
            for (int r = 1; r < 4; r++)
            {
                p = rotate90(p);
                c = rotate90(c);
                INFO("prev " << prev << " cur " << cur << " rotated " << r * 90);
                REQUIRE(decodePair(p, c) == expected);
            }
        }
    }
}

TEST_CASE("QPSKDiff round-trips an encoded stream, rotated or not", "[common][codings][differential]")
{
    std::vector<uint8_t> data = makeSymbols(1000, 3);
    std::vector<uint8_t> coded = encode(data, 2);

    for (int rotation = 0; rotation < 4; rotation++)
    {
        INFO("rotation " << rotation * 90);
        std::vector<uint8_t> rx = coded;
        for (auto &s : rx)
            for (int r = 0; r < rotation; r++)
                s = rotate90(s);

        // Prepend one dummy symbol, the first two inputs only prime the history
        rx.insert(rx.begin(), 0);

        diff::QPSKDiff dec;
        dec.swap = false;
        std::vector<uint8_t> out(rx.size() * 2, 0xFF);
        dec.work(rx.data(), rx.size(), out.data());

        for (size_t i = 0; i < data.size(); i++)
            REQUIRE((out[i * 2] << 1 | out[i * 2 + 1]) == data[i]);
        CHECK(out[data.size() * 2] == 0xFF); // nothing written past len - 2 symbols
    }
}

TEST_CASE("QPSKDiff swap flag swaps the two output bits", "[common][codings][differential]")
{
    for (int prev = 0; prev < 4; prev++)
    {
        for (int cur = 0; cur < 4; cur++)
        {
            uint8_t normal = decodePair(prev, cur, false);
            uint8_t swapped = decodePair(prev, cur, true);
            CHECK(swapped == ((normal & 1) << 1 | normal >> 1));
        }
    }
    // Swapped output is the default
    diff::QPSKDiff dec;
    CHECK(dec.swap == true);
}

TEST_CASE("QPSKDiff keeps its history across calls", "[common][codings][differential]")
{
    std::vector<uint8_t> coded = makeSymbols(300, 9);

    diff::QPSKDiff whole;
    std::vector<uint8_t> expected(coded.size() * 2);
    whole.work(coded.data(), coded.size(), expected.data());
    expected.resize((coded.size() - 2) * 2);

    diff::QPSKDiff chunked;
    std::vector<uint8_t> got;
    size_t pos = 0;
    bool first = true;
    for (size_t chunk : {5u, 1u, 94u, 200u})
    {
        std::vector<uint8_t> out(chunk * 2);
        chunked.work(&coded[pos], chunk, out.data());
        // The first call outputs chunk - 2 symbols, later calls one per input
        size_t produced = first ? chunk - 2 : chunk;
        got.insert(got.end(), out.begin(), out.begin() + produced * 2);
        pos += chunk;
        first = false;
    }
    CHECK(got == expected);
}
