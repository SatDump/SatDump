#include "image/image_lut.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdlib>

using satdump::image::Image;
namespace img = satdump::image;

TEST_CASE("image::create_lut interpolates between control points", "[image][image_lut]")
{
    Image lut = img::create_lut<uint16_t>(1, 5, 2, {0, 65535});
    REQUIRE(lut.width() == 5);
    REQUIRE(lut.height() == 1);
    REQUIRE(lut.channels() == 1);
    REQUIRE(lut.depth() == 16);
    // Output x maps to x * (points - 1) / width in the control points
    const int expected[5] = {0, 13107, 26214, 39321, 52428};
    for (int i = 0; i < 5; i++)
        CHECK(std::abs(lut.get(i) - expected[i]) <= 1);
}

TEST_CASE("image::create_lut uses planar channel data", "[image][image_lut]")
{
    // Two points, two channels: channel 0 = {0, 0}, channel 1 = {200, 200}
    Image lut = img::create_lut<uint8_t>(2, 4, 2, {0, 0, 200, 200});
    REQUIRE(lut.depth() == 8);
    REQUIRE(lut.channels() == 2);
    for (int i = 0; i < 4; i++)
    {
        CHECK(lut.get(0, i) == 0);
        CHECK(std::abs(lut.get(1, i) - 200) <= 1);
    }
}

TEST_CASE("image::LUT_jet goes from blue through green to red", "[image][image_lut]")
{
    Image jet = img::LUT_jet<uint8_t>();
    REQUIRE(jet.width() == 256);
    REQUIRE(jet.height() == 1);
    REQUIRE(jet.channels() == 3);
    REQUIRE(jet.depth() == 8);

    // Starts pure blue
    CHECK(jet.get(0, 0) == 0);
    CHECK(jet.get(1, 0) == 0);
    CHECK(jet.get(2, 0) == 255);

    // Ends (nearly) red. The last entry is not exactly the last control point,
    // as resize_bilinear maps x to x * (points - 1) / width.
    CHECK(jet.get(0, 255) >= 254);
    CHECK(jet.get(1, 255) <= 4);
    CHECK(jet.get(2, 255) == 0);

    // Middle has strong green
    CHECK(jet.get(1, 128) >= 250);

    // Red never decreases, blue never increases (allowing float truncation)
    for (int i = 1; i < 256; i++)
    {
        REQUIRE(jet.get(0, i) + 1 >= jet.get(0, i - 1));
        REQUIRE(jet.get(2, i) <= jet.get(2, i - 1) + 1);
    }
}

TEST_CASE("image::LUT_jet 16-bit matches the 8-bit version", "[image][image_lut]")
{
    Image j8 = img::LUT_jet<uint8_t>();
    Image j16 = img::LUT_jet<uint16_t>();
    REQUIRE(j16.depth() == 16);
    REQUIRE(j16.width() == 256);
    for (size_t i = 0; i < j8.size(); i++)
        REQUIRE(std::abs((j16.get(i) >> 8) - j8.get(i)) <= 1);
}

TEST_CASE("image::scale_lut places a resized LUT into a wider strip", "[image][image_lut]")
{
    Image in(8, 2, 1, 3);
    for (int c = 0; c < 3; c++)
    {
        in.set(c, 0, 10 + c);
        in.set(c, 1, 50 + c);
    }
    Image out = img::scale_lut(10, 2, 6, in);
    REQUIRE(out.width() == 10);
    REQUIRE(out.height() == 1);
    REQUIRE(out.channels() == 3);
    for (int c = 0; c < 3; c++)
    {
        const int expected[10] = {0, 0, 10 + c, 10 + c, 50 + c, 50 + c, 0, 0, 0, 0};
        for (int x = 0; x < 10; x++)
            CHECK(out.get(c, x) == expected[x]);
    }
}
