#include "image/hue_saturation.h"
#include "image/hue_saturation_json.h"
#include "image/image_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using Catch::Approx;
using satdump::image::Image;
namespace img = satdump::image;

namespace
{
    // hue_saturation works on a 0..(maxval - 1) scale, so 254 is "1.0" at 8 bits
    Image rgbPixel(int r, int g, int b)
    {
        Image im(8, 1, 1, 3);
        im.set(0, r);
        im.set(1, g);
        im.set(2, b);
        return im;
    }
} // namespace

TEST_CASE("image::rgb_to_hsl on primary, secondary and grey colours", "[image][hue_saturation]")
{
    double h, s, l;

    img::rgb_to_hsl(1, 0, 0, h, s, l);
    CHECK(h == Approx(0.0));
    CHECK(s == Approx(1.0));
    CHECK(l == Approx(0.5));

    img::rgb_to_hsl(0, 1, 0, h, s, l);
    CHECK(h == Approx(1.0 / 3.0));
    CHECK(s == Approx(1.0));
    CHECK(l == Approx(0.5));

    img::rgb_to_hsl(0, 0, 1, h, s, l);
    CHECK(h == Approx(2.0 / 3.0));

    img::rgb_to_hsl(1, 1, 0, h, s, l);
    CHECK(h == Approx(1.0 / 6.0));

    img::rgb_to_hsl(1, 0, 1, h, s, l);
    CHECK(h == Approx(5.0 / 6.0));

    // Lighter colour: l > 0.5 uses the other saturation formula
    img::rgb_to_hsl(1, 0.5, 0.5, h, s, l);
    CHECK(h == Approx(0.0));
    CHECK(l == Approx(0.75));
    CHECK(s == Approx(1.0));

    // Greys have no hue (reported as -1) and zero saturation
    img::rgb_to_hsl(0.3, 0.3, 0.3, h, s, l);
    CHECK(h == -1.0);
    CHECK(s == 0.0);
    CHECK(l == Approx(0.3));
}

TEST_CASE("image::hsl_to_rgb inverts rgb_to_hsl", "[image][hue_saturation]")
{
    const double colours[][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0.2, 0.4, 0.6}, {0.9, 0.1, 0.5}, {0.5, 0.5, 0.5}, {0.75, 0.6, 0.1}, {0, 0, 0}, {1, 1, 1}};
    for (auto &c : colours)
    {
        double h, s, l, r, g, b;
        img::rgb_to_hsl(c[0], c[1], c[2], h, s, l);
        img::hsl_to_rgb(h, s, l, r, g, b);
        INFO("rgb " << c[0] << " " << c[1] << " " << c[2]);
        CHECK(r == Approx(c[0]).margin(1e-9));
        CHECK(g == Approx(c[1]).margin(1e-9));
        CHECK(b == Approx(c[2]).margin(1e-9));
    }
}

TEST_CASE("image::HueSaturation defaults to neutral settings", "[image][hue_saturation]")
{
    img::HueSaturation hs;
    CHECK(hs.overlap == 0.0);
    for (int i = img::HUE_RANGE_ALL; i <= img::HUE_RANGE_MAGENTA; i++)
    {
        CHECK(hs.hue[i] == 0.0);
        CHECK(hs.saturation[i] == 0.0);
        CHECK(hs.lightness[i] == 0.0);
    }
}

TEST_CASE("image::HueSaturation JSON round trip", "[image][hue_saturation]")
{
    img::HueSaturation hs;
    hs.hue[img::HUE_RANGE_RED] = 0.25;
    hs.saturation[img::HUE_RANGE_ALL] = -0.5;
    hs.lightness[img::HUE_RANGE_BLUE] = 0.1;
    hs.overlap = 30;

    nlohmann::json j = hs;
    img::HueSaturation back = j.get<img::HueSaturation>();
    CHECK(back.hue[img::HUE_RANGE_RED] == 0.25);
    CHECK(back.saturation[img::HUE_RANGE_ALL] == -0.5);
    CHECK(back.lightness[img::HUE_RANGE_BLUE] == 0.1);
    CHECK(back.overlap == 30);
}

TEST_CASE("image::hue_saturation neutral settings keep colours", "[image][hue_saturation]")
{
    Image im = image_test::makePattern(8, 8, 8, 3);
    for (size_t i = 0; i < im.size(); i++)
        im.set(i, std::min(im.get(i), 254));
    Image orig = im;
    img::hue_saturation(im, img::HueSaturation());
    for (size_t i = 0; i < im.size(); i++)
        REQUIRE(std::abs(im.get(i) - orig.get(i)) <= 1);
}

TEST_CASE("image::hue_saturation global hue shift rotates red to green", "[image][hue_saturation]")
{
    // A hue value of 2/3 shifts the hue by 1/3 of a turn (120 degrees)
    Image im = rgbPixel(254, 0, 0);
    img::HueSaturation hs;
    hs.hue[img::HUE_RANGE_ALL] = 2.0 / 3.0;
    img::hue_saturation(im, hs);
    CHECK(im.get(0) <= 1);
    CHECK(im.get(1) >= 253);
    CHECK(im.get(2) <= 1);
}

TEST_CASE("image::hue_saturation range specific settings only affect that range", "[image][hue_saturation]")
{
    img::HueSaturation hs;
    hs.saturation[img::HUE_RANGE_BLUE] = -1; // desaturate blues only

    Image red = rgbPixel(254, 0, 0);
    img::hue_saturation(red, hs);
    CHECK(red.get(0) >= 253);
    CHECK(red.get(1) <= 1);
    CHECK(red.get(2) <= 1);

    Image blue = rgbPixel(0, 0, 254);
    img::hue_saturation(blue, hs);
    // Fully desaturated blue at l = 0.5 is mid grey
    for (int c = 0; c < 3; c++)
        CHECK(std::abs(blue.get(c) - 127) <= 1);
}

TEST_CASE("image::hue_saturation saturation and lightness extremes", "[image][hue_saturation]")
{
    SECTION("saturation -1 gives grey")
    {
        Image im = rgbPixel(254, 0, 0);
        img::HueSaturation hs;
        hs.saturation[img::HUE_RANGE_ALL] = -1;
        img::hue_saturation(im, hs);
        for (int c = 0; c < 3; c++)
            CHECK(std::abs(im.get(c) - 127) <= 1);
    }

    SECTION("lightness +1 gives white")
    {
        Image im = rgbPixel(100, 50, 200);
        img::HueSaturation hs;
        hs.lightness[img::HUE_RANGE_ALL] = 1;
        img::hue_saturation(im, hs);
        for (int c = 0; c < 3; c++)
            CHECK(im.get(c) >= 253);
    }

    SECTION("lightness -1 gives black")
    {
        Image im = rgbPixel(100, 50, 200);
        img::HueSaturation hs;
        hs.lightness[img::HUE_RANGE_ALL] = -1;
        img::hue_saturation(im, hs);
        for (int c = 0; c < 3; c++)
            CHECK(im.get(c) <= 1);
    }

    SECTION("greys are not affected by hue or saturation")
    {
        Image im = rgbPixel(80, 80, 80);
        img::HueSaturation hs;
        hs.hue[img::HUE_RANGE_ALL] = 0.5;
        hs.saturation[img::HUE_RANGE_ALL] = 1;
        img::hue_saturation(im, hs);
        for (int c = 0; c < 3; c++)
            CHECK(std::abs(im.get(c) - 80) <= 1);
    }
}

TEST_CASE("image::hue_saturation leaves alpha untouched and reports progress", "[image][hue_saturation]")
{
    Image im = image_test::makePattern(16, 4, 4, 4);
    Image orig = im;
    img::HueSaturation hs;
    hs.hue[img::HUE_RANGE_ALL] = 0.3;
    hs.overlap = 50;
    float progress = -1;
    img::hue_saturation(im, hs, &progress);
    for (size_t i = 0; i < 16; i++)
        REQUIRE(im.get(3, i) == orig.get(3, i));
    CHECK(progress >= 0.9f);
    CHECK(progress <= 1.0f);
}

TEST_CASE("image::hue_saturation requires at least 3 channels", "[image][hue_saturation]")
{
    auto channels = GENERATE(1, 2);
    Image im(8, 2, 2, channels);
    CHECK_THROWS(img::hue_saturation(im, img::HueSaturation()));
}
