#include "image/expression.h"
#include "image/image_test_helpers.h"
#include "image/meta.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using satdump::image::ExpressionChannel;
using satdump::image::Image;
namespace img = satdump::image;

TEST_CASE("image::generate_image_expression evaluates a single-channel expression", "[image][expression]")
{
    auto depth = GENERATE(8, 16);
    Image a = image_test::makePattern(depth, 5, 4, 1);
    Image out = img::generate_image_expression({{"a", &a}}, "a * 0.5");
    REQUIRE(out.width() == 5);
    REQUIRE(out.height() == 4);
    REQUIRE(out.channels() == 1);
    REQUIRE(out.depth() == depth);
    for (size_t i = 0; i < out.size(); i++)
        REQUIRE(std::abs(out.get(i) - a.get(i) / 2) <= 1);
}

TEST_CASE("image::generate_image_expression with several outputs and inputs", "[image][expression]")
{
    Image a(8, 2, 2, 1), b(8, 2, 2, 1);
    a.fill(51);  // 0.2
    b.fill(204); // 0.8
    Image out = img::generate_image_expression({{"a", &a}, {"b", &b}}, "a, b, a + b");
    REQUIRE(out.channels() == 3);
    for (int i = 0; i < 4; i++)
    {
        CHECK(std::abs(out.get(0, i) - 51) <= 1);
        CHECK(std::abs(out.get(1, i) - 204) <= 1);
        CHECK(std::abs(out.get(2, i) - 255) <= 1);
    }
}

TEST_CASE("image::generate_image_expression clamps results to [0, 1]", "[image][expression]")
{
    Image a(16, 3, 1, 1);
    a.set(0, 0);
    a.set(1, 32768);
    a.set(2, 65535);
    Image out = img::generate_image_expression({{"a", &a}}, "a * 4 - 1");
    CHECK(out.get(0) == 0);
    CHECK(out.get(1) == 65535);
    CHECK(out.get(2) == 65535);
}

TEST_CASE("image::generate_image_expression names multi-channel inputs", "[image][expression]")
{
    Image rgb(8, 2, 1, 3);
    for (int c = 0; c < 3; c++)
        for (int i = 0; i < 2; i++)
            rgb.set(c, i, 50 * (c + 1) + i);

    SECTION("explicit comma separated names")
    {
        Image out = img::generate_image_expression({{"r,g,b", &rgb}}, "b, g, r");
        REQUIRE(out.channels() == 3);
        for (int i = 0; i < 2; i++)
        {
            CHECK(std::abs(out.get(0, i) - rgb.get(2, i)) <= 1);
            CHECK(std::abs(out.get(1, i) - rgb.get(1, i)) <= 1);
            CHECK(std::abs(out.get(2, i) - rgb.get(0, i)) <= 1);
        }
    }

    SECTION("missing names default to <name>_<index>")
    {
        // A single token names channel 0, the others become img_2 and img_3
        Image out = img::generate_image_expression({{"img", &rgb}}, "img_3");
        REQUIRE(out.channels() == 1);
        for (int i = 0; i < 2; i++)
            CHECK(std::abs(out.get(0, i) - rgb.get(2, i)) <= 1);

        Image first = img::generate_image_expression({{"img", &rgb}}, "img");
        for (int i = 0; i < 2; i++)
            CHECK(std::abs(first.get(0, i) - rgb.get(0, i)) <= 1);
    }
}

TEST_CASE("image::generate_image_expression uses the highest input depth", "[image][expression]")
{
    Image a8(8, 2, 2, 1), b16(16, 2, 2, 1);
    a8.fill(255);
    Image out = img::generate_image_expression({{"a", &a8}, {"b", &b16}}, "a");
    CHECK(out.depth() == 16);
    for (size_t i = 0; i < out.size(); i++)
        CHECK(out.get(i) == 65535);
}

TEST_CASE("image::generate_image_expression propagates projection metadata", "[image][expression]")
{
    Image a(8, 2, 2, 1);
    img::set_metadata_proj_cfg(a, {{"type", "equirec"}});
    Image out = img::generate_image_expression({{"a", &a}}, "a");
    REQUIRE(img::has_metadata_proj_cfg(out));
    CHECK(img::get_metadata_proj_cfg(out)["type"] == "equirec");
}

TEST_CASE("image::generate_image_expression error handling", "[image][expression]")
{
    Image a(8, 2, 2, 1), wide(8, 3, 2, 1);

    CHECK_THROWS(img::generate_image_expression({}, "1"));
    CHECK_THROWS(img::generate_image_expression({{"a", &a}, {"w", &wide}}, "a"));
    CHECK_THROWS(img::generate_image_expression({{"a", &a}}, "a +"));
    CHECK_THROWS(img::generate_image_expression({{"a", &a}}, "unknown_var"));
    CHECK_THROWS(img::generate_image_expression({{"a", &a}}, "a, a, a, a, a"));
}
