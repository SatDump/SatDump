#include "image/image.h"
#include "image_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <set>
#include <utility>

using satdump::image::Image;

namespace
{
    using PixelSet = std::set<std::pair<int, int>>;

    // Coordinates of all non-zero pixels in channel 0
    PixelSet litPixels(Image &img)
    {
        PixelSet s;
        for (size_t y = 0; y < img.height(); y++)
            for (size_t x = 0; x < img.width(); x++)
                if (img.get(0, x, y) != 0)
                    s.insert({int(x), int(y)});
        return s;
    }
} // namespace

TEST_CASE("image::Image draw_pixel and get_pixel_color", "[image][image_draw]")
{
    auto depth = GENERATE(8, 16);
    Image img(depth, 3, 3, 3);
    img.draw_pixel(1, 2, {1.0, 0.0, 0.5});
    CHECK(img.get(0, 1, 2) == img.maxval());
    CHECK(img.get(1, 1, 2) == 0);
    CHECK(img.get(2, 1, 2) == img.maxval() / 2);
    CHECK(img.get(0, 0, 0) == 0);

    auto col = img.get_pixel_color(1, 2);
    REQUIRE(col.size() == 3);
    CHECK(col[0] == 1.0);
    CHECK(col[1] == 0.0);
    CHECK(col[2] == Catch::Approx(0.5).margin(1.0 / img.maxval()));

    // Extra colour components are ignored, missing ones are an error
    CHECK_NOTHROW(img.draw_pixel(0, 0, {1.0, 1.0, 1.0, 1.0}));
    CHECK_THROWS(img.draw_pixel(0, 0, {1.0, 1.0}));
}

TEST_CASE("image::Image draw_line horizontal, vertical and diagonal", "[image][image_draw]")
{
    // Lines are drawn from the start point up to, but excluding, the end point
    SECTION("horizontal")
    {
        Image img(8, 6, 6, 1);
        img.draw_line(1, 2, 4, 2, {1.0});
        CHECK(litPixels(img) == PixelSet{{1, 2}, {2, 2}, {3, 2}});
    }

    SECTION("vertical, drawn backwards")
    {
        Image img(8, 6, 6, 1);
        img.draw_line(3, 5, 3, 1, {1.0});
        CHECK(litPixels(img) == PixelSet{{3, 5}, {3, 4}, {3, 3}, {3, 2}});
    }

    SECTION("diagonal")
    {
        Image img(16, 6, 6, 1);
        img.draw_line(0, 0, 4, 4, {1.0});
        CHECK(litPixels(img) == PixelSet{{0, 0}, {1, 1}, {2, 2}, {3, 3}});
        CHECK(img.get(0, 1, 1) == 65535);
    }

    SECTION("shallow slope covers one pixel per column")
    {
        Image img(8, 10, 10, 1);
        img.draw_line(0, 0, 8, 2, {1.0});
        auto px = litPixels(img);
        REQUIRE(px.size() == 8);
        std::set<int> cols;
        for (auto &p : px)
        {
            cols.insert(p.first);
            CHECK(p.second >= 0);
            CHECK(p.second <= 2);
        }
        CHECK(cols.size() == 8);
    }

    SECTION("zero length line draws nothing")
    {
        Image img(8, 4, 4, 1);
        img.draw_line(2, 2, 2, 2, {1.0});
        CHECK(litPixels(img).empty());
    }

    SECTION("colour applies to every channel")
    {
        Image img(8, 4, 4, 3);
        img.draw_line(0, 1, 3, 1, {0.0, 1.0, 0.0});
        for (int x = 0; x < 3; x++)
        {
            CHECK(img.get(0, x, 1) == 0);
            CHECK(img.get(1, x, 1) == 255);
            CHECK(img.get(2, x, 1) == 0);
        }
    }
}

TEST_CASE("image::Image draw_line ignores lines with out of range endpoints", "[image][image_draw]")
{
    Image img(8, 5, 5, 1);
    img.draw_line(-1, 0, 3, 3, {1.0});
    img.draw_line(0, -1, 3, 3, {1.0});
    img.draw_line(0, 0, 6, 3, {1.0});
    img.draw_line(0, 0, 3, 9, {1.0});
    CHECK(litPixels(img).empty());
}

// image_draw.cpp:107: the bounds check uses `x0 > width` instead of `>=`, so a
// line starting at x == width is accepted and writes past the end of the row
// (here it wraps onto pixel (0, 1)).
TEST_CASE("image::Image draw_line rejects a start point at x == width", "[image][image_draw][!shouldfail]")
{
    Image img(8, 4, 4, 1);
    img.draw_line(4, 0, 4, 1, {1.0});
    CHECK(litPixels(img).empty());
}

TEST_CASE("image::Image draw_rectangle filled and outlined", "[image][image_draw]")
{
    SECTION("filled covers [x0, x1) x [y0, y1)")
    {
        Image img(8, 6, 6, 1);
        img.draw_rectangle(1, 1, 4, 3, {1.0}, true);
        CHECK(litPixels(img) == PixelSet{{1, 1}, {2, 1}, {3, 1}, {1, 2}, {2, 2}, {3, 2}});
    }

    SECTION("outline covers the whole perimeter of [x0, x1] x [y0, y1]")
    {
        Image img(8, 6, 6, 1);
        img.draw_rectangle(1, 1, 4, 3, {1.0}, false);
        PixelSet expected;
        for (int x = 1; x <= 4; x++)
        {
            expected.insert({x, 1});
            expected.insert({x, 3});
        }
        expected.insert({1, 2});
        expected.insert({4, 2});
        CHECK(litPixels(img) == expected);
    }
}

TEST_CASE("image::Image draw_circle hollow follows the midpoint algorithm", "[image][image_draw]")
{
    Image img(8, 11, 11, 1);
    img.draw_circle(5, 5, 2, {1.0}, false);
    PixelSet expected = {{5, 7}, {5, 3}, {7, 5}, {3, 5}, {6, 7}, {4, 7}, {6, 3}, {4, 3}, {7, 6}, {3, 6}, {7, 4}, {3, 4}};
    CHECK(litPixels(img) == expected);
}

TEST_CASE("image::Image draw_circle filled covers the centre and stays within the radius", "[image][image_draw]")
{
    Image img(8, 15, 15, 1);
    img.draw_circle(7, 7, 4, {1.0}, true);
    auto px = litPixels(img);
    CHECK(px.count({7, 7}) == 1);
    CHECK(px.count({6, 7}) == 1);
    CHECK(px.count({7, 4}) == 1);
    for (auto &p : px)
    {
        int dx = p.first - 7, dy = p.second - 7;
        CHECK(dx * dx + dy * dy <= 5 * 5);
    }
    // Roughly pi * r^2 pixels
    CHECK(px.size() > 40);
    CHECK(px.size() < 65);
}

// image_draw.cpp:250: filled circles are built from draw_line spans, which
// exclude their end point, so the right-most pixel of every span is missing and
// the filled disc does not cover its own outline (e.g. (x0 + r, y0)).
TEST_CASE("image::Image draw_circle filled covers the hollow outline", "[image][image_draw][!shouldfail]")
{
    Image hollow(8, 15, 15, 1), filled(8, 15, 15, 1);
    hollow.draw_circle(7, 7, 4, {1.0}, false);
    filled.draw_circle(7, 7, 4, {1.0}, true);
    auto outline = litPixels(hollow);
    auto disc = litPixels(filled);
    for (auto &p : outline)
        CHECK(disc.count(p) == 1);
}

TEST_CASE("image::Image draw_aa_line splits intensity across two rows", "[image][image_draw]")
{
    Image img(16, 12, 12, 1);
    img.draw_aa_line(1, 5.25f, 9, 5.25f, {1.0});

    // Coverage of each column is distributed over rows floor(y) - 1 and floor(y)
    // (the line at y lands mostly on the row above floor(y))
    for (int x = 1; x <= 9; x++)
    {
        double sum = 0;
        for (int y = 0; y < 12; y++)
        {
            double v = img.getf(0, x, y);
            if (y != 4 && y != 5)
                REQUIRE(v == 0.0);
            sum += v;
        }
        CHECK(sum == Catch::Approx(1.0).margin(2.0 / 65535));
        CHECK(img.getf(0, x, 5) == Catch::Approx(0.25).margin(1.0 / 65535));
        CHECK(img.getf(0, x, 4) == Catch::Approx(0.75).margin(1.0 / 65535));
    }
    for (int y = 0; y < 12; y++)
    {
        CHECK(img.get(0, 0, y) == 0);
        CHECK(img.get(0, 10, y) == 0);
    }
}

TEST_CASE("image::Image draw_aa_line ignores out of range endpoints", "[image][image_draw]")
{
    Image img(8, 8, 8, 1);
    img.draw_aa_line(-1, 2, 5, 2, {1.0});
    img.draw_aa_line(1, 2, 8, 2, {1.0});
    img.draw_aa_line(1, 2, 5, 8, {1.0});
    CHECK(litPixels(img).empty());
}

TEST_CASE("image::Image draw_aa_line steep lines stay near the ideal line", "[image][image_draw]")
{
    Image img(8, 12, 12, 1);
    img.draw_aa_line(5.5f, 1, 5.5f, 10, {1.0});
    auto px = litPixels(img);
    REQUIRE(!px.empty());
    for (auto &p : px)
    {
        CHECK(p.first >= 4);
        CHECK(p.first <= 5);
    }
}

TEST_CASE("image::Image draw_image copies a same-format image at an offset", "[image][image_draw]")
{
    auto depth = GENERATE(8, 16);
    Image dst(depth, 5, 4, 3);
    Image src = image_test::makePattern(depth, 2, 2, 3);
    dst.draw_image(0, src, 2, 1);
    for (int c = 0; c < 3; c++)
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 5; x++)
            {
                bool inside = x >= 2 && x < 4 && y >= 1 && y < 3;
                REQUIRE(dst.get(c, x, y) == (inside ? src.get(c, x - 2, y - 1) : 0));
            }
}

TEST_CASE("image::Image draw_image grayscale onto a single channel", "[image][image_draw]")
{
    Image dst(8, 3, 3, 3);
    Image src(8, 3, 3, 1);
    src.fill(200);
    dst.draw_image(1, src);
    for (int i = 0; i < 9; i++)
    {
        CHECK(dst.get(0, i) == 0);
        CHECK(dst.get(1, i) == 200);
        CHECK(dst.get(2, i) == 0);
    }
}

TEST_CASE("image::Image draw_image clips at every border", "[image][image_draw]")
{
    Image src(8, 3, 3, 1);
    src.fill(9);

    SECTION("bottom right")
    {
        Image dst(8, 4, 4, 1);
        dst.draw_image(0, src, 2, 3);
        PixelSet px = litPixels(dst);
        CHECK(px == PixelSet{{2, 3}, {3, 3}});
    }

    SECTION("top left with negative offsets")
    {
        Image dst(8, 4, 4, 1);
        dst.draw_image(0, src, -2, -1);
        PixelSet px = litPixels(dst);
        CHECK(px == PixelSet{{0, 0}, {0, 1}});
    }

    SECTION("larger source than destination")
    {
        Image big(8, 10, 10, 1);
        big.fill(3);
        Image dst(8, 4, 4, 1);
        dst.draw_image(0, big);
        for (int i = 0; i < 16; i++)
            REQUIRE(dst.get(i) == 3);
    }
}

TEST_CASE("image::Image draw_image validates depth and channel count", "[image][image_draw]")
{
    Image dst(8, 4, 4, 3);
    Image wrong_depth(16, 2, 2, 3);
    Image wrong_channels(8, 2, 2, 2);
    CHECK_THROWS(dst.draw_image(0, wrong_depth));
    CHECK_THROWS(dst.draw_image(0, wrong_channels));
}

TEST_CASE("image::Image draw_image_alpha blends gray+alpha onto grayscale", "[image][image_draw]")
{
    Image dst(8, 3, 1, 1);
    dst.fill(100);
    Image src(8, 3, 1, 2);
    src.fill(200);
    src.set(1, 0, 255); // opaque
    src.set(1, 1, 0);   // transparent
    src.set(1, 2, 128); // ~half
    dst.draw_image_alpha(src);
    CHECK(dst.get(0) == 200);
    CHECK(dst.get(1) == 100);
    int expected = int(200 * (128 / 255.0) + 100 * (1 - 128 / 255.0));
    CHECK(std::abs(dst.get(2) - expected) <= 1);
}

TEST_CASE("image::Image draw_image_alpha blends RGBA onto RGB and RGBA", "[image][image_draw]")
{
    auto depth = GENERATE(8, 16);
    auto dst_channels = GENERATE(3, 4);
    Image dst(depth, 2, 1, dst_channels);
    for (int c = 0; c < 3; c++)
        dst.set(c, 0, dst.maxval() / 4);
    for (int c = 0; c < 3; c++)
        dst.set(c, 1, dst.maxval() / 4);
    if (dst_channels == 4)
    {
        dst.set(3, 0, 0);
        dst.set(3, 1, dst.maxval());
    }

    Image src(depth, 2, 1, 4);
    for (int c = 0; c < 3; c++)
    {
        src.set(c, 0, src.maxval());
        src.set(c, 1, src.maxval());
    }
    src.set(3, 0, src.maxval()); // opaque
    src.set(3, 1, 0);            // transparent

    dst.draw_image_alpha(src);
    for (int c = 0; c < 3; c++)
    {
        CHECK(dst.get(c, 0) == dst.maxval());
        CHECK(dst.get(c, 1) == dst.maxval() / 4);
    }
    if (dst_channels == 4)
    {
        // Destination alpha becomes the max of both alphas
        CHECK(dst.get(3, 0) == dst.maxval());
        CHECK(dst.get(3, 1) == dst.maxval());
    }
}

TEST_CASE("image::Image draw_image_alpha with offset only touches the covered area", "[image][image_draw]")
{
    Image dst(8, 4, 4, 1);
    Image src(8, 2, 2, 2);
    src.fill(255);
    dst.draw_image_alpha(src, 3, 3);
    CHECK(litPixels(dst) == PixelSet{{3, 3}});
}

TEST_CASE("image::Image draw_image_alpha rejects invalid configurations", "[image][image_draw]")
{
    Image rgb(8, 2, 2, 3);
    Image gray(8, 2, 2, 1);
    Image ga(8, 2, 2, 2);
    Image rgba16(16, 2, 2, 4);
    CHECK_THROWS(rgb.draw_image_alpha(gray));   // source without alpha
    CHECK_THROWS(rgb.draw_image_alpha(ga));     // gray+alpha onto RGB
    CHECK_THROWS(gray.draw_image_alpha(rgba16)); // depth mismatch
    Image rgba8(8, 2, 2, 4);
    CHECK_THROWS(gray.draw_image_alpha(rgba8)); // RGBA onto gray
}
