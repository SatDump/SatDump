#include "image/brightness_contrast.h"
#include "image_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using satdump::image::Image;

TEST_CASE("image::brightness_contrast neutral settings keep values within one count", "[image][brightness_contrast]")
{
    auto depth = GENERATE(8, 16);
    Image im = image_test::makePattern(depth, 16, 16, 3);
    // Exclude the maximum value, which is covered by the [!shouldfail] test below
    for (size_t i = 0; i < im.size(); i++)
        if (im.get(i) == im.maxval())
            im.set(i, im.maxval() - 1);
    Image orig = im;
    satdump::image::brightness_contrast(im, 0, 0);
    for (size_t i = 0; i < im.size(); i++)
        REQUIRE(std::abs(im.get(i) - orig.get(i)) <= 1);
}

// brightness_contrast.cpp:11: the working scale is maxval - 1 and the result is
// clamped to it, so with neutral settings the maximum value (255 / 65535) is
// lowered to maxval - 1.
TEST_CASE("image::brightness_contrast neutral settings keep full white", "[image][brightness_contrast][!shouldfail]")
{
    auto depth = GENERATE(8, 16);
    Image im(depth, 1, 1, 1);
    im.set(0, im.maxval());
    satdump::image::brightness_contrast(im, 0, 0);
    CHECK(im.get(0) == im.maxval());
}

TEST_CASE("image::brightness_contrast brightness shifts towards white or black", "[image][brightness_contrast]")
{
    Image im(8, 2, 1, 1);
    im.set(0, 0);
    im.set(1, 254);

    SECTION("brightness +1 maps black to mid grey and keeps white")
    {
        satdump::image::brightness_contrast(im, 1, 0);
        CHECK(std::abs(im.get(0) - 127) <= 1);
        CHECK(im.get(1) == 254);
    }

    SECTION("brightness -1 halves values")
    {
        satdump::image::brightness_contrast(im, -1, 0);
        CHECK(im.get(0) == 0);
        CHECK(std::abs(im.get(1) - 127) <= 1);
    }
}

TEST_CASE("image::brightness_contrast contrast -1 flattens everything to mid grey", "[image][brightness_contrast]")
{
    auto depth = GENERATE(8, 16);
    Image im = image_test::makePattern(depth, 8, 8, 1);
    satdump::image::brightness_contrast(im, 0, -1);
    int mid = (im.maxval() - 1) / 2;
    for (size_t i = 0; i < im.size(); i++)
        REQUIRE(std::abs(im.get(i) - mid) <= 1);
}

TEST_CASE("image::brightness_contrast high contrast saturates the extremes", "[image][brightness_contrast]")
{
    Image im(16, 3, 1, 1);
    im.set(0, 1000);
    im.set(1, 32767);
    im.set(2, 64000);
    satdump::image::brightness_contrast(im, 0, 0.5f);
    CHECK(im.get(0) == 0);
    CHECK(std::abs(im.get(1) - 32767) <= 2);
    CHECK(im.get(2) == 65534);
}

TEST_CASE("image::brightness_contrast leaves the alpha channel untouched", "[image][brightness_contrast]")
{
    Image im = image_test::makePattern(8, 4, 4, 4);
    Image orig = im;
    float progress = -1;
    satdump::image::brightness_contrast(im, 0, -1, &progress);
    for (size_t i = 0; i < 16; i++)
        REQUIRE(im.get(3, i) == orig.get(3, i));
    for (int c = 0; c < 3; c++)
        for (size_t i = 0; i < 16; i++)
            REQUIRE(std::abs(im.get(c, i) - 127) <= 1);
    CHECK(progress >= 0.9f);
    CHECK(progress <= 1.0f);
}
