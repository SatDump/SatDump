#include "image/image.h"
#include "image/image_test_helpers.h"
#include "image/processing.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using satdump::image::Image;
namespace img = satdump::image;

namespace
{
    Image fromValues(int depth, int w, int h, std::vector<int> values)
    {
        Image out(depth, w, h, values.size() / (w * h));
        for (size_t i = 0; i < values.size(); i++)
            out.set(i, values[i]);
        return out;
    }
} // namespace

TEST_CASE("image::linear_invert maps v to maxval - v", "[image][processing]")
{
    auto depth = GENERATE(8, 16);
    Image orig = image_test::makePattern(depth, 4, 4, 3);
    Image inv = orig;
    img::linear_invert(inv);
    for (size_t i = 0; i < inv.size(); i++)
        REQUIRE(inv.get(i) == orig.maxval() - orig.get(i));

    // Inverting twice is the identity
    img::linear_invert(inv);
    CHECK(image_test::samePixels(inv, orig));
}

TEST_CASE("image::normalize stretches min/max to the full range", "[image][processing]")
{
    SECTION("8-bit")
    {
        // Range 51 gives an exact scaling factor of 5
        Image im = fromValues(8, 3, 1, {10, 20, 61});
        img::normalize(im);
        CHECK(im.get(0) == 0);
        CHECK(im.get(1) == 50);
        CHECK(im.get(2) == 255);
    }

    SECTION("16-bit")
    {
        // Range 257 gives an exact scaling factor of 255
        Image im = fromValues(16, 2, 2, {1000, 1100, 1257, 1000});
        img::normalize(im);
        CHECK(im.get(0) == 0);
        CHECK(im.get(1) == 25500);
        CHECK(im.get(2) == 65535);
        CHECK(im.get(3) == 0);
    }

    SECTION("min and max are taken over all channels")
    {
        Image im = fromValues(8, 1, 1, {10, 61, 20});
        img::normalize(im);
        CHECK(im.get(0) == 0);
        CHECK(im.get(1) == 255);
        CHECK(im.get(2) == 50);
    }

    SECTION("constant images are left untouched")
    {
        Image im(8, 3, 3, 1);
        im.fill(77);
        img::normalize(im);
        for (size_t i = 0; i < im.size(); i++)
            REQUIRE(im.get(i) == 77);
    }
}

TEST_CASE("image::equalize computes the cumulative histogram mapping", "[image][processing]")
{
    SECTION("two-level 8-bit image")
    {
        // cdf(0) = 2/4, cdf(255) = 4/4 -> 0 maps to round(127.5) = 128
        Image im = fromValues(8, 2, 2, {0, 0, 255, 255});
        img::equalize(im);
        CHECK(im.get(0) == 128);
        CHECK(im.get(1) == 128);
        CHECK(im.get(2) == 255);
        CHECK(im.get(3) == 255);
    }

    SECTION("four distinct 8-bit levels")
    {
        Image im = fromValues(8, 4, 1, {10, 20, 30, 40});
        img::equalize(im);
        // cdf = 1/4, 2/4, 3/4, 4/4 scaled to 255
        CHECK(im.get(0) == 64);
        CHECK(im.get(1) == 128);
        CHECK(im.get(2) == 191);
        CHECK(im.get(3) == 255);
    }

    SECTION("16-bit")
    {
        Image im = fromValues(16, 2, 1, {5, 60000});
        img::equalize(im);
        CHECK(im.get(0) == 32768); // round(1/2 * 65535)
        CHECK(im.get(1) == 65535);
    }

    SECTION("joint histogram over all channels by default")
    {
        // Channel 0 = {0, 0}, channel 1 = {255, 255}: joint cdf(0) = 1/2
        Image im = fromValues(8, 2, 1, {0, 0, 255, 255});
        img::equalize(im, false);
        CHECK(im.get(0, 0) == 128);
        CHECK(im.get(1, 0) == 255);
    }

    SECTION("per channel equalization")
    {
        Image im = fromValues(8, 2, 1, {0, 0, 255, 255});
        img::equalize(im, true);
        // Each channel is constant, so cdf(value) = 1 for both
        CHECK(im.get(0, 0) == 255);
        CHECK(im.get(0, 1) == 255);
        CHECK(im.get(1, 0) == 255);
        CHECK(im.get(1, 1) == 255);
    }

    SECTION("per channel skips the alpha channel")
    {
        Image im(8, 2, 1, 4);
        for (int c = 0; c < 4; c++)
        {
            im.set(c, 0, 10);
            im.set(c, 1, 20);
        }
        img::equalize(im, true);
        for (int c = 0; c < 3; c++)
        {
            CHECK(im.get(c, 0) == 128);
            CHECK(im.get(c, 1) == 255);
        }
        CHECK(im.get(3, 0) == 10);
        CHECK(im.get(3, 1) == 20);
    }
}

TEST_CASE("image::equalize output is monotonic in the input", "[image][processing]")
{
    Image im = image_test::makePattern(8, 16, 16, 1);
    Image orig = im;
    img::equalize(im);
    size_t mismatches = 0, first_i = 0, first_j = 0;
    for (size_t i = 0; i < im.size(); i++)
        for (size_t j = 0; j < im.size(); j++)
            if (orig.get(i) < orig.get(j) && !(im.get(i) <= im.get(j)) && mismatches++ == 0)
                first_i = i, first_j = j;
    INFO("first mismatch at samples " << first_i << " and " << first_j);
    REQUIRE(mismatches == 0);
}

TEST_CASE("image::white_balance stretches each channel between percentiles", "[image][processing]")
{
    SECTION("small image uses min/max with the default percentile")
    {
        Image im = fromValues(8, 4, 1, {50, 100, 150, 200});
        img::white_balance(im);
        CHECK(im.get(0) == 0);
        CHECK(im.get(1) == 85);
        CHECK(im.get(2) == 170);
        CHECK(im.get(3) == 255);
    }

    SECTION("channels are balanced independently")
    {
        Image im = fromValues(16, 2, 1, {1000, 2000, 30000, 40000});
        img::white_balance(im);
        CHECK(im.get(0, 0) == 0);
        CHECK(im.get(0, 1) == 65535);
        CHECK(im.get(1, 0) == 0);
        CHECK(im.get(1, 1) == 65535);
    }

    SECTION("values outside the percentiles are clipped")
    {
        // 100 samples: 1..100. A 10% percentile cuts the tails.
        Image im(8, 100, 1, 1);
        for (int i = 0; i < 100; i++)
            im.set(i, i + 1);
        img::white_balance(im, 10.0f);
        CHECK(im.get(0) == 0);
        CHECK(im.get(5) == 0);
        CHECK(im.get(99) == 255);
        CHECK(im.get(95) == 255);
        // Monotonic in between
        for (int i = 1; i < 100; i++)
            REQUIRE(im.get(i) >= im.get(i - 1));
    }
}

TEST_CASE("image::median_blur removes isolated speckles and keeps edges", "[image][processing]")
{
    auto depth = GENERATE(8, 16);

    SECTION("single bright pixel")
    {
        Image im(depth, 5, 5, 1);
        im.set(0, 2, 2, im.maxval());
        img::median_blur(im);
        for (size_t i = 0; i < im.size(); i++)
            REQUIRE(im.get(i) == 0);
    }

    SECTION("vertical step edge is preserved")
    {
        Image im(depth, 6, 4, 2);
        for (int c = 0; c < 2; c++)
            for (int y = 0; y < 4; y++)
                for (int x = 3; x < 6; x++)
                    im.set(c, x, y, 1000 % (im.maxval() + 1));
        Image orig = im;
        img::median_blur(im);
        CHECK(image_test::samePixels(im, orig));
    }

    SECTION("constant image")
    {
        Image im(depth, 4, 4, 3);
        im.fill(42);
        img::median_blur(im);
        for (size_t i = 0; i < im.size(); i++)
            REQUIRE(im.get(i) == 42);
    }
}

TEST_CASE("image::kuwahara_filter picks the most uniform quadrant", "[image][processing]")
{
    SECTION("constant image is unchanged")
    {
        Image im(8, 6, 6, 3);
        im.fill(90);
        float progress = -1;
        img::kuwahara_filter(im, &progress);
        for (size_t i = 0; i < im.size(); i++)
            REQUIRE(im.get(i) == 90);
        CHECK(progress >= 0.0f);
        CHECK(progress <= 1.0f);
    }

    SECTION("isolated spike")
    {
        Image im(8, 5, 5, 1);
        im.set(0, 2, 2, 255);
        img::kuwahara_filter(im);
        // Every 2x2 quadrant around the spike contains it, so the first one wins
        // and the spike becomes its mean 255 / 4
        CHECK(im.get(0, 2, 2) == 63);
        // Every other pixel has a spike-free quadrant with zero variance
        for (int y = 0; y < 5; y++)
            for (int x = 0; x < 5; x++)
                if (!(x == 2 && y == 2))
                    REQUIRE(im.get(0, x, y) == 0);
    }
}

TEST_CASE("image::simple_despeckle replaces bright isolated pixels", "[image][processing]")
{
    Image im(8, 5, 3, 1);
    im.fill(10);
    im.set(0, 2, 1, 200);
    img::simple_despeckle(im, 10);
    CHECK(im.get(0, 2, 1) == 10);
    // Interior pixels of the flat area are not affected
    for (int x = 0; x < 5; x++)
        CHECK(im.get(0, x, 0) == 10);
}

TEST_CASE("image::simple_despeckle below the threshold does nothing", "[image][processing]")
{
    Image im(8, 5, 3, 1);
    im.fill(10);
    im.set(0, 2, 1, 15);
    img::simple_despeckle(im, 10);
    CHECK(im.get(0, 2, 1) == 15);
}

// Bug: processing.cpp:266-268: out-of-image neighbours are treated as 0, so the
// bottom-right pixel of a perfectly uniform image looks like a speckle
// (current - right and current - below exceed the threshold) and is halved.
TEST_CASE("image::simple_despeckle leaves a uniform image unchanged", "[image][processing][!shouldfail]")
{
    Image im(8, 4, 4, 1);
    im.fill(100);
    img::simple_despeckle(im, 10);
    for (size_t i = 0; i < im.size(); i++)
        CHECK(im.get(i) == 100);
}
