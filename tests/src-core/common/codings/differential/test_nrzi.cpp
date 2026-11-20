#include "common/codings/differential/nrzi.h"
#include "helpers/random.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

namespace
{
    using test_helpers::randomBits;
} // namespace

TEST_CASE("NRZIDiff decode_bits: no level change is a 1, a change is a 0", "[common][codings][differential]")
{
    // HDLC/AX.25 style NRZI, starting from level 0
    std::vector<uint8_t> levels = {0, 0, 1, 1, 0, 1, 1, 1};
    diff::NRZIDiff dec;
    dec.decode_bits(levels.data(), levels.size());
    CHECK(levels == std::vector<uint8_t>{1, 1, 0, 1, 0, 0, 1, 1});
}

TEST_CASE("NRZIDiff decode_bits inverts NRZI encoding across chunked calls", "[common][codings][differential]")
{
    std::vector<uint8_t> data = randomBits(777, 21);
    std::vector<uint8_t> coded(data.size());
    uint8_t level = 0;
    for (size_t i = 0; i < data.size(); i++)
        coded[i] = level ^= !data[i];

    diff::NRZIDiff dec;
    size_t pos = 0;
    for (size_t chunk : {1u, 2u, 50u, 300u})
    {
        dec.decode_bits(&coded[pos], chunk);
        pos += chunk;
    }
    dec.decode_bits(&coded[pos], coded.size() - pos);
    CHECK(coded == data);
}

TEST_CASE("NRZIDiff output is always a single bit", "[common][codings][differential]")
{
    std::vector<uint8_t> levels = {0, 1, 0, 1, 1};
    diff::NRZIDiff dec;
    dec.decode_bits(levels.data(), levels.size());
    for (uint8_t b : levels)
        CHECK(b <= 1);
}
