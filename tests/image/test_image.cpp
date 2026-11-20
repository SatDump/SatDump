#include "image/image.h"
#include "image/meta.h"
#include "image/processing.h"
#include "image_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstring>

using satdump::image::Image;

namespace
{
    // Planar test image where each sample encodes its own coordinates
    Image makeCoordImage(int depth, int w, int h, int channels)
    {
        Image img(depth, w, h, channels);
        for (int c = 0; c < channels; c++)
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                    img.set(c, x, y, x + 10 * y + 100 * c);
        return img;
    }
} // namespace

TEST_CASE("image::Image default constructor is a null image", "[image][image_class]")
{
    Image img;
    CHECK(img.raw_data() == nullptr);
    CHECK(img.size() == 0);
    CHECK(img.width() == 0);
    CHECK(img.height() == 0);
    CHECK(img.channels() == 0);
    CHECK(img.depth() == 0);
    CHECK(img.maxval() == 0);
    CHECK(img.typesize() == 0);
}

TEST_CASE("image::Image init sets geometry, type size and max value", "[image][image_class]")
{
    auto depth = GENERATE(1, 8, 10, 12, 16);
    auto channels = GENERATE(1, 2, 3, 4);
    INFO("depth " << depth << " channels " << channels);

    Image img(depth, 5, 3, channels);
    CHECK(img.width() == 5);
    CHECK(img.height() == 3);
    CHECK(img.channels() == channels);
    CHECK(img.depth() == depth);
    CHECK(img.size() == size_t(5 * 3 * channels));
    CHECK(img.typesize() == (depth > 8 ? 2 : 1));
    CHECK(img.maxval() == (1 << depth) - 1);
    REQUIRE(img.raw_data() != nullptr);

    // Freshly initialised buffers are zeroed
    for (size_t i = 0; i < img.size(); i++)
        REQUIRE(img.get(i) == 0);
}

TEST_CASE("image::Image buffer constructor copies the buffer", "[image][image_class]")
{
    SECTION("8-bit")
    {
        uint8_t buf[6] = {1, 2, 3, 250, 251, 255};
        Image img(buf, 8, 3, 2, 1);
        buf[0] = 99; // Must not alias
        CHECK(img.get(0) == 1);
        CHECK(img.get(0, 2, 1) == 255);
        CHECK(img.get(0, 0, 1) == 250);
    }

    SECTION("16-bit")
    {
        uint16_t buf[4] = {0, 1000, 65535, 42};
        Image img(buf, 16, 2, 1, 2);
        CHECK(img.get(0, 0) == 0);
        CHECK(img.get(0, 1) == 1000);
        CHECK(img.get(1, 0) == 65535);
        CHECK(img.get(1, 1) == 42);
    }
}

TEST_CASE("image::Image copy constructor and assignment make deep copies", "[image][image_class]")
{
    Image a = image_test::makePattern(16, 4, 3, 3);
    Image b(a);
    REQUIRE(image_test::samePixels(a, b));
    CHECK(a.raw_data() != b.raw_data());
    b.set(0, 123);
    CHECK(a.get(0) != 123);

    Image c(8, 1, 1, 1);
    c = a;
    REQUIRE(image_test::samePixels(a, c));
    CHECK(c.raw_data() != a.raw_data());
}

// image.cpp:30: operator= only copies when the source has data, so assigning a
// null image leaves the destination's old buffer in place instead of emptying it.
TEST_CASE("image::Image assigning a null image empties the destination", "[image][image_class][!shouldfail]")
{
    Image a(8, 4, 4, 1);
    a = Image();
    CHECK(a.size() == 0);
    CHECK(a.width() == 0);
}

TEST_CASE("image::Image clear resets everything", "[image][image_class]")
{
    Image img(16, 4, 4, 3);
    img.clear();
    CHECK(img.raw_data() == nullptr);
    CHECK(img.size() == 0);
    CHECK(img.width() == 0);
    CHECK(img.height() == 0);
    CHECK(img.channels() == 0);
    CHECK(img.depth() == 0);
    CHECK(img.maxval() == 0);

    // Can be re-initialised afterwards
    img.init(8, 2, 2, 1);
    CHECK(img.size() == 4);
}

TEST_CASE("image::Image init on an existing image reallocates and zeroes", "[image][image_class]")
{
    Image img = image_test::makePattern(8, 4, 4, 1);
    img.init(16, 2, 3, 2);
    CHECK(img.depth() == 16);
    CHECK(img.width() == 2);
    CHECK(img.height() == 3);
    CHECK(img.size() == 12);
    for (size_t i = 0; i < img.size(); i++)
        REQUIRE(img.get(i) == 0);
}

TEST_CASE("image::Image get/set index variants address planar layout", "[image][image_class]")
{
    auto depth = GENERATE(8, 16);
    Image img(depth, 3, 2, 2);

    img.set(1, 2, 1, 77); // channel 1, x 2, y 1
    CHECK(img.get(1 * 6 + 1 * 3 + 2) == 77);
    CHECK(img.get(1, 1 * 3 + 2) == 77);
    CHECK(img.get(1, 2, 1) == 77);

    img.set(0, 4, 55);
    CHECK(img.get(0, 1, 1) == 55);
    CHECK(img.get(4) == 55);
}

TEST_CASE("image::Image get/set store full range at each depth", "[image][image_class]")
{
    Image i8(8, 2, 1, 1);
    i8.set(0, 255);
    i8.set(1, 0);
    CHECK(i8.get(0) == 255);
    CHECK(i8.get(1) == 0);

    Image i16(16, 2, 1, 1);
    i16.set(0, 65535);
    i16.set(1, 256);
    CHECK(i16.get(0) == 65535);
    CHECK(i16.get(1) == 256);
}

TEST_CASE("image::Image setf/getf scale to and from [0,1]", "[image][image_class]")
{
    Image i8(8, 3, 1, 1);
    i8.setf(0, 1.0);
    i8.setf(1, 0.0);
    i8.setf(2, 0.5);
    CHECK(i8.get(0) == 255);
    CHECK(i8.get(1) == 0);
    CHECK(i8.get(2) == 127); // 127.5 truncated
    CHECK(i8.getf(0) == 1.0);
    CHECK(i8.getf(1) == 0.0);

    Image i16(16, 2, 1, 1);
    i16.setf(0, 0, 0, 1.0);
    i16.set(1, 32768);
    CHECK(i16.get(0) == 65535);
    CHECK(i16.getf(0, 1) == double(32768) / 65535.0);
    CHECK(i16.getf(0, 1, 0) == double(32768) / 65535.0);
}

TEST_CASE("image::Image clamp and clampf bound values", "[image][image_class]")
{
    Image i8(8, 1, 1, 1);
    CHECK(i8.clamp(-5) == 0);
    CHECK(i8.clamp(0) == 0);
    CHECK(i8.clamp(128) == 128);
    CHECK(i8.clamp(255) == 255);
    CHECK(i8.clamp(256) == 255);

    Image i16(16, 1, 1, 1);
    CHECK(i16.clamp(70000) == 65535);
    CHECK(i16.clamp(256) == 256);
    CHECK(i16.clamp(-1) == 0);

    Image i12(12, 1, 1, 1);
    CHECK(i12.clamp(5000) == 4095);

    CHECK(i8.clampf(-0.1) == 0.0);
    CHECK(i8.clampf(0.25) == 0.25);
    CHECK(i8.clampf(1.5) == 1.0);
}

TEST_CASE("image::Image to_rgb from grayscale replicates the channel", "[image][image_class][conversion]")
{
    auto depth = GENERATE(8, 16);
    Image img = image_test::makePattern(depth, 4, 3, 1);
    Image orig = img;
    img.to_rgb();
    REQUIRE(img.channels() == 3);
    REQUIRE(img.depth() == depth);
    for (int c = 0; c < 3; c++)
        for (size_t i = 0; i < 12; i++)
            REQUIRE(img.get(c, i) == orig.get(i));
}

TEST_CASE("image::Image to_rgb from RGBA drops alpha, RGB is unchanged", "[image][image_class][conversion]")
{
    auto depth = GENERATE(8, 16);
    Image rgba = image_test::makePattern(depth, 3, 3, 4);
    Image orig = rgba;
    rgba.to_rgb();
    REQUIRE(rgba.channels() == 3);
    for (int c = 0; c < 3; c++)
        for (size_t i = 0; i < 9; i++)
            REQUIRE(rgba.get(c, i) == orig.get(c, i));

    Image rgb = image_test::makePattern(depth, 3, 3, 3);
    Image orig_rgb = rgb;
    rgb.to_rgb();
    CHECK(image_test::samePixels(rgb, orig_rgb));

    // Gray+alpha images are not handled by to_rgb and stay as they are
    Image ga = image_test::makePattern(depth, 3, 3, 2);
    ga.to_rgb();
    CHECK(ga.channels() == 2);
}

TEST_CASE("image::Image to_rgba from grayscale and RGB adds opaque alpha", "[image][image_class][conversion]")
{
    auto depth = GENERATE(8, 16);
    auto channels = GENERATE(1, 3);
    Image img = image_test::makePattern(depth, 4, 2, channels);
    Image orig = img;
    img.to_rgba();
    REQUIRE(img.channels() == 4);
    for (size_t i = 0; i < 8; i++)
    {
        for (int c = 0; c < 3; c++)
            REQUIRE(img.get(c, i) == orig.get(channels == 1 ? 0 : c, i));
        REQUIRE(img.get(3, i) == img.maxval());
    }

    // Already RGBA: no-op
    Image again = img;
    again.to_rgba();
    CHECK(image_test::samePixels(again, img));
}

TEST_CASE("image::Image to_rgba from 8-bit gray+alpha keeps alpha", "[image][image_class][conversion]")
{
    Image img = image_test::makePattern(8, 4, 2, 2);
    Image orig = img;
    img.to_rgba();
    REQUIRE(img.channels() == 4);
    for (size_t i = 0; i < 8; i++)
    {
        for (int c = 0; c < 3; c++)
            REQUIRE(img.get(c, i) == orig.get(0, i));
        REQUIRE(img.get(3, i) == orig.get(1, i));
    }
}

// image.cpp:174: the alpha plane source offset is w*h bytes instead of
// w*h*type_size bytes, so 16-bit gray+alpha images get half gray / half alpha
// data in the new alpha channel.
TEST_CASE("image::Image to_rgba from 16-bit gray+alpha keeps alpha", "[image][image_class][conversion][!shouldfail]")
{
    Image img = image_test::makePattern(16, 4, 2, 2);
    Image orig = img;
    img.to_rgba();
    REQUIRE(img.channels() == 4);
    for (size_t i = 0; i < 8; i++)
        CHECK(img.get(3, i) == orig.get(1, i));
}

TEST_CASE("image::Image to8bits / to16bits shift by 8 bits", "[image][image_class][conversion]")
{
    Image i16(16, 4, 1, 1);
    i16.set(0, 0);
    i16.set(1, 255);
    i16.set(2, 256);
    i16.set(3, 65535);
    Image i8 = i16.to8bits();
    REQUIRE(i8.depth() == 8);
    REQUIRE(i8.size() == 4);
    CHECK(i8.get(0) == 0);
    CHECK(i8.get(1) == 0);
    CHECK(i8.get(2) == 1);
    CHECK(i8.get(3) == 255);

    // Upscaling is a plain shift, so 255 becomes 65280 rather than 65535
    Image back = i8.to16bits();
    REQUIRE(back.depth() == 16);
    CHECK(back.get(1) == 0);
    CHECK(back.get(2) == 256);
    CHECK(back.get(3) == 65280);

    // Same-depth conversions return an identical copy
    Image same8 = i8.to8bits();
    CHECK(image_test::samePixels(same8, i8));
    Image same16 = i16.to16bits();
    CHECK(image_test::samePixels(same16, i16));

    CHECK(i16.to_depth(8).depth() == 8);
    CHECK(i8.to_depth(16).depth() == 16);
    CHECK(i8.to_depth(12).depth() == 16);
}

TEST_CASE("image::Image to8bits / to16bits throw on unsupported depth", "[image][image_class][conversion]")
{
    Image i12(12, 2, 2, 1);
    CHECK_THROWS(i12.to8bits());
    CHECK_THROWS(i12.to16bits());
}

TEST_CASE("image::Image crop extracts a region in every channel", "[image][image_class][crop]")
{
    auto depth = GENERATE(8, 16);
    Image img = makeCoordImage(depth, 5, 4, 3);
    img.crop(1, 1, 4, 3);
    REQUIRE(img.width() == 3);
    REQUIRE(img.height() == 2);
    REQUIRE(img.channels() == 3);
    REQUIRE(img.size() == 18);
    for (int c = 0; c < 3; c++)
        for (int y = 0; y < 2; y++)
            for (int x = 0; x < 3; x++)
                REQUIRE(img.get(c, x, y) == (x + 1) + 10 * (y + 1) + 100 * c);
}

TEST_CASE("image::Image crop on X only keeps full height", "[image][image_class][crop]")
{
    Image img = makeCoordImage(8, 5, 3, 1);
    img.crop(2, 5);
    REQUIRE(img.width() == 3);
    REQUIRE(img.height() == 3);
    for (int y = 0; y < 3; y++)
        for (int x = 0; x < 3; x++)
            REQUIRE(img.get(0, x, y) == x + 2 + 10 * y);
}

TEST_CASE("image::Image crop of the full area is the identity", "[image][image_class][crop]")
{
    Image img = image_test::makePattern(16, 6, 4, 2);
    Image orig = img;
    img.crop(0, 0, 6, 4);
    CHECK(image_test::samePixels(img, orig));
}

TEST_CASE("image::Image crop_to returns a region and leaves source untouched", "[image][image_class][crop]")
{
    auto depth = GENERATE(8, 16);
    Image img = makeCoordImage(depth, 5, 4, 2);
    Image orig = img;

    Image out = img.crop_to(2, 1, 5, 4);
    CHECK(image_test::samePixels(img, orig));
    REQUIRE(out.width() == 3);
    REQUIRE(out.height() == 3);
    REQUIRE(out.depth() == depth);
    for (int c = 0; c < 2; c++)
        for (int y = 0; y < 3; y++)
            for (int x = 0; x < 3; x++)
                REQUIRE(out.get(c, x, y) == (x + 2) + 10 * (y + 1) + 100 * c);

    Image cols = img.crop_to(1, 2);
    REQUIRE(cols.width() == 1);
    REQUIRE(cols.height() == 4);
    for (int y = 0; y < 4; y++)
        CHECK(cols.get(1, 0, y) == 1 + 10 * y + 100);

    // crop and crop_to agree
    Image inplace = orig;
    inplace.crop(2, 1, 5, 4);
    CHECK(image_test::samePixels(inplace, out));
}

TEST_CASE("image::Image mirror flips along the requested axes", "[image][image_class][mirror]")
{
    auto depth = GENERATE(8, 16);
    const int w = 4, h = 3;
    Image orig = makeCoordImage(depth, w, h, 2);

    SECTION("mirror X reverses columns")
    {
        Image img = orig;
        img.mirror(true, false);
        for (int c = 0; c < 2; c++)
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                    REQUIRE(img.get(c, x, y) == orig.get(c, w - 1 - x, y));
    }

    SECTION("mirror Y reverses rows")
    {
        Image img = orig;
        img.mirror(false, true);
        for (int c = 0; c < 2; c++)
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                    REQUIRE(img.get(c, x, y) == orig.get(c, x, h - 1 - y));
    }

    SECTION("mirroring twice is the identity")
    {
        Image img = orig;
        img.mirror(true, true);
        img.mirror(true, true);
        CHECK(image_test::samePixels(img, orig));
    }

    SECTION("no flags is a no-op")
    {
        Image img = orig;
        img.mirror(false, false);
        CHECK(image_test::samePixels(img, orig));
    }
}

TEST_CASE("image::Image mirror on a 1x1 image is a no-op", "[image][image_class][mirror]")
{
    Image img(8, 1, 1, 1);
    img.set(0, 42);
    img.mirror(true, true);
    CHECK(img.get(0) == 42);
}

TEST_CASE("image::Image resize upscales by pixel duplication", "[image][image_class][resize]")
{
    auto depth = GENERATE(8, 16);
    Image img(depth, 2, 2, 2);
    for (int c = 0; c < 2; c++)
        for (int i = 0; i < 4; i++)
            img.set(c, i, 10 * c + i + 1);

    Image up = img.resize_to(4, 8);
    REQUIRE(up.width() == 4);
    REQUIRE(up.height() == 8);
    REQUIRE(up.channels() == 2);
    for (int c = 0; c < 2; c++)
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 4; x++)
                REQUIRE(up.get(c, x, y) == img.get(c, x / 2, y / 4));

    Image inplace = img;
    inplace.resize(4, 8);
    CHECK(image_test::samePixels(inplace, up));
}

TEST_CASE("image::Image resize downscales by picking source pixels", "[image][image_class][resize]")
{
    Image img = makeCoordImage(8, 4, 4, 1);
    img.resize(2, 2);
    REQUIRE(img.width() == 2);
    REQUIRE(img.height() == 2);
    CHECK(img.get(0, 0, 0) == 0);
    CHECK(img.get(0, 1, 0) == 2);
    CHECK(img.get(0, 0, 1) == 20);
    CHECK(img.get(0, 1, 1) == 22);

    // Same size is the identity
    Image p = image_test::makePattern(16, 5, 3, 3);
    Image same = p.resize_to(5, 3);
    CHECK(image_test::samePixels(same, p));
}

TEST_CASE("image::Image resize_bilinear keeps constant images constant", "[image][image_class][resize]")
{
    auto depth = GENERATE(8, 16);
    Image img(depth, 4, 4, 3);
    img.fill(depth == 8 ? 100 : 30000);
    img.resize_bilinear(9, 7);
    REQUIRE(img.width() == 9);
    REQUIRE(img.height() == 7);
    REQUIRE(img.channels() == 3);
    int expected = depth == 8 ? 100 : 30000;
    for (size_t i = 0; i < img.size(); i++)
    {
        // Float weights summing to 1 can truncate one count lower
        REQUIRE(img.get(i) <= expected);
        REQUIRE(img.get(i) >= expected - 1);
    }
}

TEST_CASE("image::Image resize_bilinear interpolates a horizontal ramp", "[image][image_class][resize]")
{
    Image img(16, 2, 2, 1);
    img.set(0, 0, 0, 0);
    img.set(0, 1, 0, 1000);
    img.set(0, 0, 1, 0);
    img.set(0, 1, 1, 1000);
    img.resize_bilinear(4, 2);
    // x_scale = (2 - 1) / 4, so output x maps to x/4 in the source
    CHECK(img.get(0, 0, 0) == 0);
    CHECK(img.get(0, 1, 0) == 250);
    CHECK(img.get(0, 2, 0) == 500);
    CHECK(img.get(0, 3, 0) == 750);
}

TEST_CASE("image::Image resize_bilinear text mode saturates non-zero pixels", "[image][image_class][resize]")
{
    Image img(8, 4, 4, 1);
    img.set(0, 1, 1, 10);
    img.resize_bilinear(8, 8, true);
    bool any_max = false;
    for (size_t i = 0; i < img.size(); i++)
    {
        REQUIRE((img.get(i) == 0 || img.get(i) == 255));
        any_max |= img.get(i) == 255;
    }
    CHECK(any_max);
}

TEST_CASE("image::Image get_pixel_bilinear interpolates interior points", "[image][image_class][resize]")
{
    Image img(16, 3, 3, 1);
    // Plane value = 100 * x + 1000 * y, bilinear interpolation is exact on it
    for (int y = 0; y < 3; y++)
        for (int x = 0; x < 3; x++)
            img.set(0, x, y, 100 * x + 1000 * y);

    CHECK(img.get_pixel_bilinear(0, 0, 0) == 0);
    CHECK(img.get_pixel_bilinear(0, 0.5, 0) == 50);
    CHECK(img.get_pixel_bilinear(0, 0, 0.5) == 500);
    CHECK(img.get_pixel_bilinear(0, 0.5, 0.5) == 550);
    CHECK(img.get_pixel_bilinear(0, 1.25, 1.5) == 1625);

    // On the last column/row no neighbour exists, so the base pixel is returned
    CHECK(img.get_pixel_bilinear(0, 2.0, 0.5) == 200);
    CHECK(img.get_pixel_bilinear(0, 0.5, 2.0) == 2000);
}

TEST_CASE("image::Image get_pixel_bilinear on RGBA weights by alpha", "[image][image_class][resize]")
{
    Image img(8, 3, 3, 4);
    // Fully opaque left column (value 200), transparent elsewhere
    for (int y = 0; y < 3; y++)
        for (int x = 0; x < 3; x++)
        {
            img.set(0, x, y, x == 0 ? 200 : 0);
            img.set(3, x, y, x == 0 ? 255 : 0);
        }
    // Halfway between an opaque and a fully transparent pixel, the colour
    // comes from the opaque one only
    CHECK(img.get_pixel_bilinear(0, 0.5, 0) == 200);
    CHECK(img.get_pixel_bilinear(3, 0.5, 0) == 127);
}

TEST_CASE("image::Image fill and fill_color", "[image][image_class][fill]")
{
    auto depth = GENERATE(8, 16);
    Image img(depth, 3, 2, 3);
    img.fill(7);
    for (size_t i = 0; i < img.size(); i++)
        REQUIRE(img.get(i) == 7);

    img.fill_color({1.0, 0.0, 0.5});
    for (size_t i = 0; i < 6; i++)
    {
        REQUIRE(img.get(0, i) == img.maxval());
        REQUIRE(img.get(1, i) == 0);
        REQUIRE(img.get(2, i) == img.maxval() / 2);
    }

    // Colour vectors must have at least one entry per channel
    CHECK_THROWS(img.fill_color({1.0, 1.0}));
}

TEST_CASE("image::imemcpy copies samples and validates arguments", "[image][image_class]")
{
    auto depth = GENERATE(8, 16);
    Image src = image_test::makePattern(depth, 4, 4, 1);
    Image dst(depth, 4, 4, 1);
    satdump::image::imemcpy(dst, 2, src, 5, 6);
    for (size_t i = 0; i < 16; i++)
    {
        if (i >= 2 && i < 8)
            REQUIRE(dst.get(i) == src.get(i + 3));
        else
            REQUIRE(dst.get(i) == 0);
    }

    Image other_depth(depth == 8 ? 16 : 8, 4, 4, 1);
    CHECK_THROWS(satdump::image::imemcpy(other_depth, 0, src, 0, 1));
    CHECK_THROWS(satdump::image::imemcpy(dst, 10, src, 0, 7));
    CHECK_THROWS(satdump::image::imemcpy(dst, 0, src, 10, 7));
    CHECK_NOTHROW(satdump::image::imemcpy(dst, 0, src, 0, 16));
}

TEST_CASE("image::image_to_rgba packs ABGR words for every channel count", "[image][image_class][conversion]")
{
    SECTION("grayscale 8-bit")
    {
        Image img(8, 2, 1, 1);
        img.set(0, 0x12);
        img.set(1, 0xFF);
        uint32_t out[2];
        satdump::image::image_to_rgba(img, out);
        CHECK(out[0] == 0xFF121212u);
        CHECK(out[1] == 0xFFFFFFFFu);
    }

    SECTION("gray+alpha 8-bit")
    {
        Image img(8, 1, 1, 2);
        img.set(0, 0, 0x40);
        img.set(1, 0, 0x80);
        uint32_t out[1];
        satdump::image::image_to_rgba(img, out);
        CHECK(out[0] == 0x80404040u);
    }

    SECTION("RGB 16-bit is shifted down to 8 bits")
    {
        Image img(16, 1, 1, 3);
        img.set(0, 0, 0x1200);
        img.set(1, 0, 0x3400);
        img.set(2, 0, 0x56FF);
        uint32_t out[1];
        satdump::image::image_to_rgba(img, out);
        CHECK(out[0] == 0xFF563412u);
    }

    SECTION("RGBA 8-bit")
    {
        Image img(8, 1, 1, 4);
        img.set(0, 0, 0x01);
        img.set(1, 0, 0x02);
        img.set(2, 0, 0x03);
        img.set(3, 0, 0x04);
        uint32_t out[1];
        satdump::image::image_to_rgba(img, out);
        CHECK(out[0] == 0x04030201u);
    }
}

TEST_CASE("image::rotate by 90/180/270 degrees", "[image][processing][rotate]")
{
    auto depth = GENERATE(8, 16);
    const int w = 4, h = 3;
    Image orig = makeCoordImage(depth, w, h, 2);

    SECTION("0 is a no-op")
    {
        Image img = orig;
        satdump::image::rotate(img, 0);
        CHECK(image_test::samePixels(img, orig));
    }

    SECTION("90 rotates clockwise")
    {
        Image img = orig;
        satdump::image::rotate(img, 90);
        REQUIRE(img.width() == size_t(h));
        REQUIRE(img.height() == size_t(w));
        for (int c = 0; c < 2; c++)
            for (int y = 0; y < w; y++)
                for (int x = 0; x < h; x++)
                    REQUIRE(img.get(c, x, y) == orig.get(c, y, h - 1 - x));
    }

    SECTION("270 rotates counter-clockwise")
    {
        Image img = orig;
        satdump::image::rotate(img, 270);
        REQUIRE(img.width() == size_t(h));
        REQUIRE(img.height() == size_t(w));
        for (int c = 0; c < 2; c++)
            for (int y = 0; y < w; y++)
                for (int x = 0; x < h; x++)
                    REQUIRE(img.get(c, x, y) == orig.get(c, w - 1 - y, x));
    }

    SECTION("180 equals mirroring both axes")
    {
        Image img = orig;
        satdump::image::rotate(img, 180);
        Image m = orig;
        m.mirror(true, true);
        CHECK(image_test::samePixels(img, m));
    }

    SECTION("four 90 degree turns are the identity")
    {
        Image img = orig;
        for (int i = 0; i < 4; i++)
            satdump::image::rotate(img, 90);
        CHECK(image_test::samePixels(img, orig));
    }

    SECTION("90 then 270 is the identity")
    {
        Image img = orig;
        satdump::image::rotate(img, 90);
        satdump::image::rotate(img, 270);
        CHECK(image_test::samePixels(img, orig));
    }

    SECTION("two 90 turns equal 180")
    {
        Image a = orig, b = orig;
        satdump::image::rotate(a, 90);
        satdump::image::rotate(a, 90);
        satdump::image::rotate(b, 180);
        CHECK(image_test::samePixels(a, b));
    }
}

TEST_CASE("image::rotate rejects other angles", "[image][processing][rotate]")
{
    Image img(8, 2, 2, 1);
    CHECK_THROWS(satdump::image::rotate(img, 45));
    CHECK_THROWS(satdump::image::rotate(img, -90));
    CHECK_THROWS(satdump::image::rotate(img, 360));
}
