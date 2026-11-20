#include "utils/majority_law.h"
#include <catch2/catch_test_macros.hpp>

using namespace satdump;

TEST_CASE("majority_law_vec votes per bit", "[utils][majority_law]")
{
    using bytes = std::vector<unsigned char>;

    CHECK(majority_law_vec({{0xFF}, {0x0F}, {0x00}}) == bytes{0x0F});
    CHECK(majority_law_vec({{0xAA, 0x01}, {0xAA, 0x01}, {0x55, 0xFE}}) == bytes{0xAA, 0x01});
    CHECK(majority_law_vec({{0x12, 0x34}}) == bytes{0x12, 0x34});

    SECTION("a corrupted copy is outvoted")
    {
        bytes good = {0xDE, 0xAD, 0xBE, 0xEF};
        bytes bad = {0x00, 0xFF, 0xBE, 0x10};
        CHECK(majority_law_vec({good, bad, good}) == good);
    }

    SECTION("ties resolve to one")
    {
        CHECK(majority_law_vec({{0xF0}, {0x0F}}) == bytes{0xFF});
    }

    SECTION("empty elements give an empty result")
    {
        CHECK(majority_law_vec({{}, {}}).empty());
    }
}

TEST_CASE("majority_law votes per bit on fixed-size arrays", "[utils][majority_law]")
{
    const uint8_t input[3][2] = {
        {0xFF, 0xAA},
        {0x0F, 0xAA},
        {0x00, 0x55},
    };
    uint8_t output[2] = {0, 0};

    majority_law(input, output);

    CHECK(output[0] == 0x0F);
    CHECK(output[1] == 0xAA);
}
