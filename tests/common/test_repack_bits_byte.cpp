#include "common/repack_bits_byte.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
    // Expand bytes into one bit per byte, MSB first
    std::vector<uint8_t> toBits(const std::vector<uint8_t> &bytes)
    {
        std::vector<uint8_t> bits;
        for (uint8_t byte : bytes)
            for (int b = 7; b >= 0; b--)
                bits.push_back((byte >> b) & 1);
        return bits;
    }
} // namespace

TEST_CASE("RepackBitsByte packs bits MSB first", "[common][repack_bits_byte]")
{
    RepackBitsByte repacker;
    std::vector<uint8_t> bits = toBits({0xA5, 0x3C, 0xFF, 0x00});
    std::vector<uint8_t> out(4);

    int n = repacker.work(bits.data(), bits.size(), out.data());

    REQUIRE(n == 4);
    CHECK(out == std::vector<uint8_t>{0xA5, 0x3C, 0xFF, 0x00});
}

TEST_CASE("RepackBitsByte only uses the lowest bit of each input byte", "[common][repack_bits_byte]")
{
    RepackBitsByte repacker;
    // 0xFF / 0x81 / 0x03 all have LSB 1; 0xFE / 0x80 have LSB 0
    std::vector<uint8_t> bits = {0xFF, 0xFE, 0x81, 0x80, 0x03, 0xFE, 0x80, 0x01};
    uint8_t out = 0;

    REQUIRE(repacker.work(bits.data(), bits.size(), &out) == 1);
    CHECK(out == 0b10101001);
}

TEST_CASE("RepackBitsByte keeps partial bytes across calls", "[common][repack_bits_byte]")
{
    RepackBitsByte repacker;
    std::vector<uint8_t> bits = toBits({0x12, 0x34, 0x56});
    std::vector<uint8_t> out(3);
    int total = 0;

    // Feed 5, then 7, then 12 bits
    total += repacker.work(bits.data(), 5, out.data() + total);
    CHECK(total == 0);
    total += repacker.work(bits.data() + 5, 7, out.data() + total);
    CHECK(total == 1);
    total += repacker.work(bits.data() + 12, 12, out.data() + total);

    REQUIRE(total == 3);
    CHECK(out == std::vector<uint8_t>{0x12, 0x34, 0x56});
}

TEST_CASE("RepackBitsByte with empty or short input produces nothing", "[common][repack_bits_byte]")
{
    RepackBitsByte repacker;
    uint8_t out = 0xEE;
    uint8_t bits[7] = {1, 1, 1, 1, 1, 1, 1};

    CHECK(repacker.work(bits, 0, &out) == 0);
    CHECK(repacker.work(bits, 7, &out) == 0);
    CHECK(out == 0xEE);

    // The eighth bit completes the pending byte
    uint8_t last = 0;
    CHECK(repacker.work(&last, 1, &out) == 1);
    CHECK(out == 0xFE);
}
