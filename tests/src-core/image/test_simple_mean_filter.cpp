#include "image/image_test_helpers.h"
#include "image/simple_mean_filter.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using satdump::image::Image;
namespace img = satdump::image;

TEST_CASE("image::simple_mean_filter keeps constant images constant", "[image][simple_mean_filter]")
{
    auto mode = GENERATE(img::SMOOTH, img::SHARPEN);
    auto depth = GENERATE(8, 16);
    auto radius = GENERATE(1, 2, 4);
    Image im(depth, 7, 5, 2);
    im.fill(depth == 8 ? 100 : 20000);
    float progress = -1;
    img::simple_mean_filter(im, radius, mode, &progress);
    int expected = depth == 8 ? 100 : 20000;
    for (size_t i = 0; i < im.size(); i++)
        REQUIRE(std::abs(im.get(i) - expected) <= 1);
    CHECK(progress >= 0.0f);
    CHECK(progress <= 1.0f);
}

TEST_CASE("image::simple_mean_filter EDGE mode is zero on constant images", "[image][simple_mean_filter]")
{
    Image im(8, 5, 5, 1);
    im.fill(80);
    img::simple_mean_filter(im, 2, img::EDGE);
    for (size_t i = 0; i < im.size(); i++)
        REQUIRE(im.get(i) == 0);
}

TEST_CASE("image::simple_mean_filter SMOOTH spreads a spike, SHARPEN boosts it", "[image][simple_mean_filter]")
{
    Image spike(8, 5, 5, 1);
    spike.set(0, 2, 2, 255);

    SECTION("smooth")
    {
        Image im = spike;
        img::simple_mean_filter(im, 1, img::SMOOTH);
        // The 2x2 averaging window [x - 1, x] x [y - 1, y] holds the spike for
        // pixels (2..3, 2..3), each getting a quarter of it
        CHECK(std::abs(im.get(0, 2, 2) - 63) <= 1);
        CHECK(std::abs(im.get(0, 3, 3) - 63) <= 1);
        CHECK(im.get(0, 0, 0) == 0);
        CHECK(im.get(0, 4, 4) == 0);
        // Total intensity is roughly preserved
        int sum = 0;
        for (size_t i = 0; i < im.size(); i++)
            sum += im.get(i);
        CHECK(sum >= 250);
        CHECK(sum <= 255);
    }

    SECTION("sharpen saturates the spike and keeps the background black")
    {
        Image im = spike;
        img::simple_mean_filter(im, 1, img::SHARPEN);
        CHECK(im.get(0, 2, 2) == 255);
        CHECK(im.get(0, 0, 0) == 0);
    }
}

TEST_CASE("image::simple_mean_filter EDGE responds to a ramp", "[image][simple_mean_filter]")
{
    // Increasing ramp: every pixel is >= the mean of its window, so the output
    // is the local difference (half a step)
    Image im(8, 6, 1, 1);
    for (int x = 0; x < 6; x++)
        im.set(0, x, 0, 20 * x);
    img::simple_mean_filter(im, 1, img::EDGE);
    CHECK(im.get(0, 0, 0) == 0);
    for (int x = 1; x < 6; x++)
        CHECK(std::abs(im.get(0, x, 0) - 10) <= 1);
}

// Bug: simple_mean_filter.cpp:10-13: the averaging window is [x - r, x + r) and
// [y - r, y + r), so it is not centred on the pixel. Smoothing a single spike
// shifts it towards +x/+y instead of spreading it symmetrically.
TEST_CASE("image::simple_mean_filter SMOOTH is symmetric around the pixel", "[image][simple_mean_filter][!shouldfail]")
{
    Image im(8, 7, 7, 1);
    im.set(0, 3, 3, 255);
    img::simple_mean_filter(im, 1, img::SMOOTH);
    CHECK(im.get(0, 2, 3) == im.get(0, 4, 3));
    CHECK(im.get(0, 3, 2) == im.get(0, 3, 4));
    CHECK(im.get(0, 2, 2) == im.get(0, 4, 4));
}
