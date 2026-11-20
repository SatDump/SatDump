#include "image/adaptive_despeckle.h"
#include "image/adaptive_despeckle_json.h"
#include "image_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using satdump::image::AdaptiveDespeckleConfig;
using satdump::image::Image;

// Note: adaptive_despeckle picks a random pixel among those sharing the median
// luminance (rand()). Every case below only has one candidate value, so the
// results are deterministic regardless of the random choice.

TEST_CASE("image::AdaptiveDespeckleConfig defaults and JSON round trip", "[image][adaptive_despeckle]")
{
    AdaptiveDespeckleConfig cfg;
    CHECK(cfg.filter_type == AdaptiveDespeckleConfig::ADAPTIVE);
    CHECK(cfg.radius == 3);
    CHECK(cfg.black_level == 0);
    CHECK(cfg.white_level == 255);

    cfg.filter_type = AdaptiveDespeckleConfig::RECURSIVE;
    cfg.radius = 5;
    cfg.black_level = 7;
    cfg.white_level = 200;
    nlohmann::json j = cfg;
    auto back = j.get<AdaptiveDespeckleConfig>();
    CHECK(back.filter_type == AdaptiveDespeckleConfig::RECURSIVE);
    CHECK(back.radius == 5);
    CHECK(back.black_level == 7);
    CHECK(back.white_level == 200);
}

TEST_CASE("image::adaptive_despeckle leaves a uniform image unchanged", "[image][adaptive_despeckle]")
{
    auto type = GENERATE(AdaptiveDespeckleConfig::ADAPTIVE, AdaptiveDespeckleConfig::RECURSIVE);
    auto depth = GENERATE(8, 16);
    auto channels = GENERATE(1, 3);
    Image im(depth, 8, 7, channels);
    im.fill(depth == 8 ? 100 : 25000);
    Image orig = im;

    AdaptiveDespeckleConfig cfg;
    cfg.filter_type = type;
    float progress = -1;
    satdump::image::adaptive_despeckle(im, cfg, &progress);
    CHECK(image_test::samePixels(im, orig));
    CHECK(progress >= 0.0f);
    CHECK(progress <= 1.0f);
}

TEST_CASE("image::adaptive_despeckle removes white and black speckles", "[image][adaptive_despeckle]")
{
    auto type = GENERATE(AdaptiveDespeckleConfig::ADAPTIVE, AdaptiveDespeckleConfig::RECURSIVE);
    auto depth = GENERATE(8, 16);
    Image im(depth, 9, 9, 1);
    int bg = depth == 8 ? 100 : 25700;
    im.fill(bg);
    im.set(0, 4, 4, im.maxval()); // white speckle, >= white_level
    im.set(0, 1, 6, 0);           // black speckle, <= black_level

    AdaptiveDespeckleConfig cfg;
    cfg.filter_type = type;
    satdump::image::adaptive_despeckle(im, cfg);
    for (size_t i = 0; i < im.size(); i++)
        REQUIRE(im.get(i) == bg);
}

TEST_CASE("image::adaptive_despeckle replaces whole RGB pixels", "[image][adaptive_despeckle]")
{
    Image im(8, 7, 7, 3);
    for (int i = 0; i < 49; i++)
    {
        im.set(0, i, 120);
        im.set(1, i, 60);
        im.set(2, i, 30);
    }
    for (int c = 0; c < 3; c++)
        im.set(c, 3, 3, 255); // white speckle

    satdump::image::adaptive_despeckle(im, AdaptiveDespeckleConfig());
    for (int i = 0; i < 49; i++)
    {
        REQUIRE(im.get(0, i) == 120);
        REQUIRE(im.get(1, i) == 60);
        REQUIRE(im.get(2, i) == 30);
    }
}

TEST_CASE("image::adaptive_despeckle keeps pixels when everything is outside the levels", "[image][adaptive_despeckle]")
{
    // With no pixel strictly between the black and white levels, the median
    // search has no candidates and every pixel keeps its own value.
    Image im(8, 6, 6, 1);
    for (size_t i = 0; i < im.size(); i++)
        im.set(i, (i % 3 == 0) ? 255 : 0);
    Image orig = im;
    satdump::image::adaptive_despeckle(im, AdaptiveDespeckleConfig());
    CHECK(image_test::samePixels(im, orig));
}
