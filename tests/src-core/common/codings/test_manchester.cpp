#include "common/codings/manchester.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

namespace
{
    // G. E. Thomas convention: 1 -> "10", 0 -> "01", MSB first, two output bytes per input byte
    std::vector<uint8_t> manchesterEncode(const std::vector<uint8_t> &data)
    {
        std::vector<uint8_t> out;
        for (uint8_t byte : data)
        {
            uint16_t word = 0;
            for (int b = 7; b >= 0; b--)
            {
                int bit = (byte >> b) & 1;
                word = word << 2 | (bit ? 0b10 : 0b01);
            }
            out.push_back(word >> 8);
            out.push_back(word & 0xFF);
        }
        return out;
    }
} // namespace

TEST_CASE("manchesterDecoder decodes known G. E. Thomas symbol pairs", "[common][codings][manchester]")
{
    std::vector<uint8_t> out(4, 0);

    uint8_t allOnes[2] = {0xAA, 0xAA}; // 10 10 10 10 ...
    CHECK(manchesterDecoder(allOnes, 2, out.data()) == 1);
    CHECK(out[0] == 0xFF);

    uint8_t allZeros[2] = {0x55, 0x55}; // 01 01 01 01 ...
    CHECK(manchesterDecoder(allZeros, 2, out.data()) == 1);
    CHECK(out[0] == 0x00);

    // 0xA5 = 1010 0101 -> 10 01 10 01 | 01 10 01 10
    uint8_t mixed[2] = {0x99, 0x66};
    manchesterDecoder(mixed, 2, out.data());
    CHECK(out[0] == 0xA5);

    // The first input byte carries the high nibble
    uint8_t nibbles[2] = {0xAA, 0x55};
    manchesterDecoder(nibbles, 2, out.data());
    CHECK(out[0] == 0xF0);
}

TEST_CASE("manchesterDecoder round-trips every byte value", "[common][codings][manchester]")
{
    std::vector<uint8_t> data(256);
    for (int i = 0; i < 256; i++)
        data[i] = i;

    std::vector<uint8_t> encoded = manchesterEncode(data);
    std::vector<uint8_t> decoded(256, 0);
    int n = manchesterDecoder(encoded.data(), encoded.size(), decoded.data());

    REQUIRE(n == 256);
    CHECK(decoded == data);
}

TEST_CASE("manchesterDecoder only samples the first half of each symbol", "[common][codings][manchester]")
{
    // Invalid "11" and "00" pairs decode from their first half-bit
    uint8_t invalid[2] = {0xFF, 0x00};
    uint8_t out = 0x5A;
    manchesterDecoder(invalid, 2, &out);
    CHECK(out == 0xF0);
}

TEST_CASE("manchesterDecoder with zero length writes nothing", "[common][codings][manchester]")
{
    uint8_t in[2] = {0xAA, 0xAA};
    uint8_t out = 0x42;
    CHECK(manchesterDecoder(in, 0, &out) == 0);
    CHECK(out == 0x42);
}
