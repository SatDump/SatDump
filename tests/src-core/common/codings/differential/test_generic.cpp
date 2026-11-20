#include "common/codings/differential/generic.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

namespace
{
    std::vector<uint8_t> makeSymbols(int count, unsigned modulus, uint32_t seed)
    {
        std::vector<uint8_t> s(count);
        for (auto &v : s)
        {
            seed = seed * 1664525u + 1013904223u;
            v = (seed >> 16) % modulus;
        }
        return s;
    }

    // Differential encoder: running sum modulo M
    std::vector<uint8_t> diffEncode(const std::vector<uint8_t> &data, unsigned modulus)
    {
        std::vector<uint8_t> out(data.size());
        unsigned acc = 0;
        for (size_t i = 0; i < data.size(); i++)
            out[i] = acc = (acc + data[i]) % modulus;
        return out;
    }
} // namespace

TEST_CASE("GenericDiff outputs (x[n] - x[n-1]) mod M with a one-sample history lag", "[common][codings][differential]")
{
    diff::GenericDiff dec(4);
    std::vector<uint8_t> in = {0, 1, 3, 2, 2, 0, 1};
    std::vector<uint8_t> out(in.size(), 0xFF);

    // The first call keeps the last two samples as history, so it outputs size - 2 values
    int n = dec.work(in.data(), in.size(), out.data());
    REQUIRE(n == 5);
    CHECK(std::vector<uint8_t>(out.begin(), out.begin() + 5) == std::vector<uint8_t>{1, 2, 3, 0, 2});

    // Following calls output one value per input, starting with the delayed difference
    uint8_t more[2] = {3, 3};
    n = dec.work(more, 2, out.data());
    REQUIRE(n == 2);
    CHECK(out[0] == 1); // 1 - 0
    CHECK(out[1] == 2); // 3 - 1
}

TEST_CASE("GenericDiff decodes a differentially encoded stream", "[common][codings][differential]")
{
    for (unsigned modulus : {2u, 4u, 8u})
    {
        INFO("modulus " << modulus);
        std::vector<uint8_t> data = makeSymbols(500, modulus, modulus);
        std::vector<uint8_t> coded = diffEncode(data, modulus);

        diff::GenericDiff dec(modulus);
        std::vector<uint8_t> out(coded.size());
        int n = dec.work(coded.data(), coded.size(), out.data());
        REQUIRE(n == (int)coded.size() - 2);
        for (int i = 0; i < n; i++)
            REQUIRE(out[i] == data[i + 1]);
    }
}

TEST_CASE("GenericDiff chunked processing matches a single call", "[common][codings][differential]")
{
    std::vector<uint8_t> coded = makeSymbols(400, 4, 77);

    diff::GenericDiff whole(4);
    std::vector<uint8_t> expected(coded.size());
    int expectedCount = whole.work(coded.data(), coded.size(), expected.data());
    expected.resize(expectedCount);

    diff::GenericDiff chunked(4);
    std::vector<uint8_t> got;
    size_t pos = 0;
    for (size_t chunk : {2u, 1u, 1u, 50u, 7u, 139u})
    {
        std::vector<uint8_t> out(chunk + 2);
        int n = chunked.work(&coded[pos], chunk, out.data());
        got.insert(got.end(), out.begin(), out.begin() + n);
        pos += chunk;
    }
    std::vector<uint8_t> out(coded.size());
    int n = chunked.work(&coded[pos], coded.size() - pos, out.data());
    got.insert(got.end(), out.begin(), out.begin() + n);

    CHECK(got == expected);
}

TEST_CASE("GenericDiff output is invariant to a constant offset of the input", "[common][codings][differential]")
{
    // Adding a constant (a constellation rotation) is invisible after differential decoding
    std::vector<uint8_t> coded = makeSymbols(200, 4, 5);
    std::vector<uint8_t> rotated(coded.size());
    for (size_t i = 0; i < coded.size(); i++)
        rotated[i] = (coded[i] + 1) % 4;

    diff::GenericDiff a(4), b(4);
    std::vector<uint8_t> outA(coded.size()), outB(coded.size());
    int na = a.work(coded.data(), coded.size(), outA.data());
    int nb = b.work(rotated.data(), rotated.size(), outB.data());
    REQUIRE(na == nb);
    CHECK(outA == outB);
}

// Bug: generic.cpp:19 computes (input[i] - input[i - 1]) % modulus with an unsigned modulus,
// so a negative difference is converted to a huge unsigned value first. That is only
// correct when the modulus is a power of two; e.g. (0 - 2) mod 3 gives 2 instead of 1.
TEST_CASE("GenericDiff handles negative differences for a non power-of-two modulus", "[common][codings][differential][!shouldfail]")
{
    diff::GenericDiff dec(3);
    std::vector<uint8_t> in = {2, 0, 1, 0};
    std::vector<uint8_t> out(4, 0xFF);
    int n = dec.work(in.data(), in.size(), out.data());
    REQUIRE(n == 2);
    CHECK(out[0] == 1); // (0 - 2) mod 3
    CHECK(out[1] == 1); // (1 - 0) mod 3
}
