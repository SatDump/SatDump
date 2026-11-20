#include "image/image_test_helpers.h"
#include "image/image_utils.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstdlib>

using satdump::image::Image;
namespace img = satdump::image;

namespace
{
    Image filled16(int w, int h, int channels, int value)
    {
        Image im(16, w, h, channels);
        im.fill(value);
        return im;
    }
} // namespace

TEST_CASE("image::make_manyimg_composite tiles images row by row", "[image][image_utils]")
{
    auto depth = GENERATE(8, 16);
    int calls = 0;
    Image out = img::make_manyimg_composite(2, 2, 3,
                                            [&](int i)
                                            {
                                                calls++;
                                                Image im(depth, 3, 2, 1);
                                                im.fill(i + 1);
                                                return im;
                                            });
    REQUIRE(out.width() == 6);
    REQUIRE(out.height() == 4);
    REQUIRE(out.depth() == depth);
    REQUIRE(out.channels() == 1);
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 6; x++)
        {
            int tile = (y / 2) * 2 + (x / 3);
            int expected = tile < 3 ? tile + 1 : 0; // Fourth slot stays empty
            REQUIRE(out.get(0, x, y) == expected);
        }
    // Image 0 is requested once for the size and once for drawing
    CHECK(calls == 4);
}

TEST_CASE("image::make_manyimg_composite with colour images", "[image][image_utils]")
{
    Image out = img::make_manyimg_composite(3, 1, 3,
                                            [](int i)
                                            {
                                                Image im(8, 1, 1, 3);
                                                for (int c = 0; c < 3; c++)
                                                    im.set(c, 0, 10 * i + c);
                                                return im;
                                            });
    REQUIRE(out.width() == 3);
    REQUIRE(out.height() == 1);
    for (int i = 0; i < 3; i++)
        for (int c = 0; c < 3; c++)
            CHECK(out.get(c, i, 0) == 10 * i + c);
}

TEST_CASE("image::blend_images averages non-zero layers", "[image][image_utils]")
{
    std::vector<Image> layers;
    layers.push_back(Image(16, 2, 1, 1));
    layers.push_back(Image(16, 2, 1, 1));
    layers[0].set(0, 0);
    layers[0].set(1, 1000);
    layers[1].set(0, 2000);
    layers[1].set(1, 3000);

    Image out = img::blend_images(layers);
    REQUIRE(out.depth() == 16);
    REQUIRE(out.width() == 2);
    REQUIRE(out.channels() == 1);
    // Zero samples are treated as "no data" and excluded from the average
    CHECK(std::abs(out.get(0) - 2000) <= 1);
    CHECK(std::abs(out.get(1) - 2000) <= 1);
}

TEST_CASE("image::blend_images RGBA weights colours by alpha", "[image][image_utils]")
{
    std::vector<Image> layers;
    layers.push_back(filled16(1, 1, 4, 0));
    layers.push_back(filled16(1, 1, 4, 0));
    for (int c = 0; c < 3; c++)
    {
        layers[0].set(c, 0, 10000);
        layers[1].set(c, 0, 30000);
    }

    SECTION("opaque over transparent")
    {
        layers[0].set(3, 0, 65535);
        layers[1].set(3, 0, 0);
        Image out = img::blend_images(layers);
        for (int c = 0; c < 3; c++)
            CHECK(std::abs(out.get(c, 0) - 10000) <= 1);
        CHECK(out.get(3, 0) == 65535);
    }

    SECTION("both opaque")
    {
        layers[0].set(3, 0, 65535);
        layers[1].set(3, 0, 65535);
        Image out = img::blend_images(layers);
        for (int c = 0; c < 3; c++)
            CHECK(std::abs(out.get(c, 0) - 20000) <= 1);
        CHECK(out.get(3, 0) == 65535);
    }
}

TEST_CASE("image::blend_images uses the smallest common size", "[image][image_utils]")
{
    std::vector<Image> layers;
    layers.push_back(filled16(4, 3, 1, 100));
    layers.push_back(filled16(2, 5, 1, 300));
    Image out = img::blend_images(layers);
    CHECK(out.width() == 2);
    CHECK(out.height() == 3);
}

TEST_CASE("image::blend_images rejects empty input and non 16-bit images", "[image][image_utils]")
{
    std::vector<Image> none;
    CHECK_THROWS(img::blend_images(none));

    std::vector<Image> eight;
    eight.push_back(Image(8, 2, 2, 1));
    CHECK_THROWS(img::blend_images(eight));
}

TEST_CASE("image::merge_images_opacity blends two RGB images", "[image][image_utils]")
{
    Image a = filled16(2, 2, 3, 10000);
    Image b = filled16(2, 2, 3, 30000);

    Image half = img::merge_images_opacity(a, b, 0.5f);
    REQUIRE(half.channels() == 3);
    for (size_t i = 0; i < half.size(); i++)
        REQUIRE(std::abs(half.get(i) - 20000) <= 1);

    Image none = img::merge_images_opacity(a, b, 0.0f);
    for (size_t i = 0; i < none.size(); i++)
        REQUIRE(std::abs(none.get(i) - 10000) <= 1);

    Image full = img::merge_images_opacity(a, b, 1.0f);
    for (size_t i = 0; i < full.size(); i++)
        REQUIRE(std::abs(full.get(i) - 30000) <= 1);
}

TEST_CASE("image::merge_images_opacity composites over a transparent RGBA base", "[image][image_utils]")
{
    Image base = filled16(1, 1, 4, 0); // fully transparent
    Image top = filled16(1, 1, 3, 40000);
    Image out = img::merge_images_opacity(base, top, 0.5f);
    REQUIRE(out.channels() == 4);
    // Only the top layer contributes colour; alpha is the top layer's opacity
    for (int c = 0; c < 3; c++)
        CHECK(std::abs(out.get(c, 0) - 40000) <= 1);
    CHECK(std::abs(out.get(3, 0) - 32767) <= 1);
}

TEST_CASE("image::merge_images_opacity uses the smallest size and requires 16-bit", "[image][image_utils]")
{
    Image a = filled16(4, 2, 3, 1);
    Image b = filled16(3, 5, 3, 1);
    Image out = img::merge_images_opacity(a, b, 0.5f);
    CHECK(out.width() == 3);
    CHECK(out.height() == 2);

    Image a8(8, 2, 2, 3), b8(8, 2, 2, 3);
    CHECK_THROWS(img::merge_images_opacity(a8, b8, 0.5f));
    CHECK_THROWS(img::merge_images_opacity(a, b8, 0.5f));
}

// Bug: image_utils.cpp:122-128: pixels are addressed by a linear index computed for
// the output (minimum) size, so when the inputs have different widths the rows
// of the wider image are misaligned in the result.
TEST_CASE("image::merge_images_opacity keeps rows aligned for inputs of different widths", "[image][image_utils][!shouldfail]")
{
    Image a(16, 3, 2, 3);
    for (int c = 0; c < 3; c++)
        for (int x = 0; x < 3; x++)
            a.set(c, x, 1, 60000); // second row bright
    Image b(16, 2, 2, 3);
    Image out = img::merge_images_opacity(a, b, 0.0f);
    REQUIRE(out.width() == 2);
    for (int x = 0; x < 2; x++)
    {
        CHECK(out.get(0, x, 0) == 0);
        CHECK(std::abs(out.get(0, x, 1) - 60000) <= 1);
    }
}

// Bug: image_utils.cpp:72-88: same linear-index issue as merge_images_opacity, rows
// of wider layers are misaligned when layers differ in width.
TEST_CASE("image::blend_images keeps rows aligned for layers of different widths", "[image][image_utils][!shouldfail]")
{
    std::vector<Image> layers;
    layers.push_back(Image(16, 3, 2, 1));
    for (int x = 0; x < 3; x++)
    {
        layers[0].set(0, x, 0, 1000);
        layers[0].set(0, x, 1, 5000);
    }
    layers.push_back(Image(16, 2, 2, 1)); // all zero, ignored by the average
    Image out = img::blend_images(layers);
    REQUIRE(out.width() == 2);
    for (int x = 0; x < 2; x++)
    {
        CHECK(std::abs(out.get(0, x, 0) - 1000) <= 1);
        CHECK(std::abs(out.get(0, x, 1) - 5000) <= 1);
    }
}
