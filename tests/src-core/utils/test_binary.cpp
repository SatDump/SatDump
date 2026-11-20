#include "utils/binary.h"
#include <catch2/catch_test_macros.hpp>

using namespace satdump;

TEST_CASE("getBit reads individual bits", "[utils][binary]")
{
    uint8_t v = 0b10000001;
    CHECK(getBit(v, 0));
    CHECK_FALSE(getBit(v, 1));
    CHECK(getBit(v, 7));

    uint32_t w = 1u << 31;
    CHECK(getBit(w, 31));
    CHECK_FALSE(getBit(w, 30));
}

TEST_CASE("unsigned_to_bitvec is MSB first", "[utils][binary]")
{
    CHECK(unsigned_to_bitvec<uint8_t>(0xA5) == std::vector<uint8_t>{1, 0, 1, 0, 0, 1, 0, 1});

    auto bits16 = unsigned_to_bitvec<uint16_t>(0x8001);
    REQUIRE(bits16.size() == 16);
    CHECK(bits16.front() == 1);
    CHECK(bits16.back() == 1);
    CHECK(bits16[1] == 0);
}

TEST_CASE("oversample_vector repeats each element", "[utils][binary]")
{
    CHECK(oversample_vector<int>({1, 2}, 3) == std::vector<int>{1, 1, 1, 2, 2, 2});
    CHECK(oversample_vector<int>({1, 2}, 1) == std::vector<int>{1, 2});
    CHECK(oversample_vector<int>({}, 4).empty());
}

TEST_CASE("swap_endian reverses byte order", "[utils][binary]")
{
    CHECK(swap_endian<uint16_t>(0x1234) == 0x3412);
    CHECK(swap_endian<uint32_t>(0x11223344) == 0x44332211);
    CHECK(swap_endian<uint64_t>(0x0102030405060708ull) == 0x0807060504030201ull);
    CHECK(swap_endian<uint8_t>(0xAB) == 0xAB);
    CHECK(swap_endian(swap_endian<uint32_t>(0xDEADBEEF)) == 0xDEADBEEF);
}

TEST_CASE("double_buffer_to_float converts every element", "[utils][binary]")
{
    double in[3] = {1.5, -2.0, 0.25};
    CHECK(double_buffer_to_float(in, 3) == std::vector<float>{1.5f, -2.0f, 0.25f});
    CHECK(double_buffer_to_float(in, 0).empty());
}

TEST_CASE("count_ones counts set bits", "[utils][binary]")
{
    CHECK(count_ones(0x00) == 0);
    CHECK(count_ones(0xFF) == 8);
    CHECK(count_ones(0xA5) == 4);
    CHECK(count_ones(0x80) == 1);
}

TEST_CASE("reverseBits mirrors a byte", "[utils][binary]")
{
    CHECK(reverseBits(0x01) == 0x80);
    CHECK(reverseBits(0xF0) == 0x0F);
    CHECK(reverseBits(0x12) == 0x48);
    CHECK(reverseBits(0xA5) == 0xA5);

    for (int i = 0; i < 256; i++)
        REQUIRE(reverseBits(reverseBits(i)) == i);
}

TEST_CASE("reverse16Bits mirrors a 16-bit word", "[utils][binary]")
{
    CHECK(reverse16Bits(0x0001) == 0x8000);
    CHECK(reverse16Bits(0x1234) == 0x2C48);
    CHECK(reverse16Bits(0xFFFF) == 0xFFFF);
    CHECK(reverse16Bits(reverse16Bits(0xBEEF)) == 0xBEEF);
}
