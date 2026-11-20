#include "image/text.h"
#include "image_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <string>

using satdump::image::Image;
using satdump::image::TextDrawer;

namespace
{
    std::string fontPath(const std::string &name = "font.ttf") { return std::string(SATDUMP_TEST_SOURCE_DIR) + "/resources/fonts/" + name; }

    struct Bounds
    {
        int count = 0;
        int x0 = 1 << 30, y0 = 1 << 30, x1 = -1, y1 = -1;
    };

    Bounds litBounds(Image &img, int channel)
    {
        Bounds b;
        for (size_t y = 0; y < img.height(); y++)
            for (size_t x = 0; x < img.width(); x++)
                if (img.get(channel, x, y) != 0)
                {
                    b.count++;
                    b.x0 = std::min<int>(b.x0, x);
                    b.y0 = std::min<int>(b.y0, y);
                    b.x1 = std::max<int>(b.x1, x);
                    b.y1 = std::max<int>(b.y1, y);
                }
        return b;
    }
} // namespace

TEST_CASE("image::TextDrawer without a font draws nothing", "[image][text]")
{
    TextDrawer drawer;
    CHECK_FALSE(drawer.font_ready());
    drawer.init_font(fontPath("does_not_exist.ttf"));
    CHECK_FALSE(drawer.font_ready());

    Image im(8, 32, 32, 1);
    CHECK(drawer.draw_text(im, 0, 0, {1.0}, 16, "Hello") == 0);
    CHECK(litBounds(im, 0).count == 0);
}

TEST_CASE("image::TextDrawer draws text inside the expected area", "[image][text]")
{
    auto font = GENERATE(std::string("font.ttf"), std::string("Roboto-Medium.ttf"), std::string("LiberationSerif-Regular.ttf"));
    INFO("font " << font);
    TextDrawer drawer;
    drawer.init_font(fontPath(font));
    REQUIRE(drawer.font_ready());

    Image im(8, 200, 60, 1);
    int advance = drawer.draw_text(im, 10, 10, {1.0}, 24, "Hello");
    CHECK(advance > 24);
    CHECK(advance < 150);

    Bounds b = litBounds(im, 0);
    REQUIRE(b.count > 20);
    CHECK(b.x0 >= 10);
    CHECK(b.x1 <= 10 + advance + 4);
    CHECK(b.y0 >= 10);
    CHECK(b.y1 < 10 + 30);
}

TEST_CASE("image::TextDrawer advance grows with text length and size", "[image][text]")
{
    TextDrawer drawer;
    drawer.init_font(fontPath());
    REQUIRE(drawer.font_ready());
    Image im(8, 400, 100, 1);

    int one = drawer.draw_text(im, 0, 0, {1.0}, 20, "M");
    int two = drawer.draw_text(im, 0, 0, {1.0}, 20, "MM");
    CHECK(one > 0);
    CHECK(std::abs(two - 2 * one) <= 1);

    int big = drawer.draw_text(im, 0, 0, {1.0}, 40, "M");
    CHECK(big > one);

    // Glyph cache is size specific: drawing at the first size again gives the same result
    CHECK(drawer.draw_text(im, 0, 0, {1.0}, 20, "M") == one);

    CHECK(drawer.draw_text(im, 0, 0, {1.0}, 20, "") == 0);
}

TEST_CASE("image::TextDrawer newline moves down and resets the advance", "[image][text]")
{
    TextDrawer drawer;
    drawer.init_font(fontPath());
    REQUIRE(drawer.font_ready());

    Image single(8, 200, 120, 1), multi(8, 200, 120, 1);
    int adv_b = drawer.draw_text(single, 5, 5, {1.0}, 20, "B");
    int adv_multi = drawer.draw_text(multi, 5, 5, {1.0}, 20, "AAAA\nB");
    // Only the last line contributes to the returned advance
    CHECK(adv_multi == adv_b);

    Bounds bs = litBounds(single, 0), bm = litBounds(multi, 0);
    CHECK(bm.y1 > bs.y1 + 10);
}

TEST_CASE("image::TextDrawer colours each channel", "[image][text]")
{
    auto depth = GENERATE(8, 16);
    TextDrawer drawer;
    drawer.init_font(fontPath());
    REQUIRE(drawer.font_ready());

    Image im(depth, 100, 40, 3);
    drawer.draw_text(im, 2, 2, {1.0, 0.0, 0.0}, 24, "X");
    Bounds r = litBounds(im, 0);
    CHECK(r.count > 10);
    CHECK(litBounds(im, 1).count == 0);
    CHECK(litBounds(im, 2).count == 0);

    // Fully covered glyph pixels reach the full requested intensity
    int max = 0;
    for (size_t i = 0; i < 100 * 40; i++)
        max = std::max(max, im.get(0, i));
    CHECK(max >= im.maxval() - 1);
}

TEST_CASE("image::TextDrawer clips text running off the image", "[image][text]")
{
    TextDrawer drawer;
    drawer.init_font(fontPath());
    REQUIRE(drawer.font_ready());

    // Drawing near the right and bottom edges must not wrap onto other rows
    Image im(8, 40, 20, 1);
    drawer.draw_text(im, 30, 10, {1.0}, 24, "WWWW");
    Bounds b = litBounds(im, 0);
    CHECK(b.x0 >= 30);
}
