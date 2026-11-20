#include "image/image_test_helpers.h"
#include "image/io.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstdio>
#include <png.h>

using satdump::image::Image;
namespace img = satdump::image;

namespace
{
    // Writes a PNG directly with libpng, for colour types save_png cannot produce.
    // rows holds already-interleaved 8-bit samples.
    void writeRawPng(const std::string &path, int w, int h, int color_type, const std::vector<uint8_t> &rows, const std::vector<png_color> &palette = {})
    {
        FILE *fp = fopen(path.c_str(), "wb");
        REQUIRE(fp != nullptr);
        png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        png_infop info = png_create_info_struct(png);
        png_init_io(png, fp);
        png_set_IHDR(png, info, w, h, 8, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        if (!palette.empty())
            png_set_PLTE(png, info, palette.data(), palette.size());
        png_write_info(png, info);
        size_t stride = rows.size() / h;
        for (int y = 0; y < h; y++)
            png_write_row(png, rows.data() + y * stride);
        png_write_end(png, nullptr);
        png_destroy_write_struct(&png, &info);
        fclose(fp);
    }
} // namespace

TEST_CASE("image::save_png / load_png round-trip exactly", "[image][io][png]")
{
    auto depth = GENERATE(8, 16);
    auto channels = GENERATE(1, 3, 4);
    auto fast = GENERATE(true, false);
    INFO("depth " << depth << " channels " << channels << " fast " << fast);

    image_test::TempDir dir;
    std::string path = dir.file("rt.png");
    Image in = image_test::makePattern(depth, 13, 7, channels);
    img::save_png(in, path, fast);
    REQUIRE(std::filesystem::exists(path));

    Image out;
    img::load_png(out, path);
    REQUIRE(image_test::sameShape(out, in));
    CHECK(image_test::samePixels(out, in));
}

TEST_CASE("image::save_png / load_png 1x1 image", "[image][io][png]")
{
    image_test::TempDir dir;
    std::string path = dir.file("one.png");
    Image in(16, 1, 1, 3);
    in.set(0, 65535);
    in.set(1, 1);
    in.set(2, 256);
    img::save_png(in, path);
    Image out;
    img::load_png(out, path);
    CHECK(image_test::samePixels(out, in));
}

TEST_CASE("image::load_png from a memory buffer", "[image][io][png]")
{
    auto depth = GENERATE(8, 16);
    auto channels = GENERATE(1, 3, 4);
    image_test::TempDir dir;
    std::string path = dir.file("buf.png");
    Image in = image_test::makePattern(depth, 9, 5, channels);
    img::save_png(in, path);
    std::vector<uint8_t> data = image_test::readBytes(path);

    Image out;
    img::load_png(out, data.data(), data.size());
    REQUIRE(image_test::sameShape(out, in));
    CHECK(image_test::samePixels(out, in));

    // Same through the generic dispatcher
    Image out2;
    img::load_img(out2, data.data(), data.size());
    CHECK(image_test::samePixels(out2, in));
}

TEST_CASE("image::load_png reads gray+alpha files", "[image][io][png]")
{
    image_test::TempDir dir;
    std::string path = dir.file("ga.png");
    // 2x1 pixels: (10, 255), (200, 0)
    writeRawPng(path, 2, 1, PNG_COLOR_TYPE_GRAY_ALPHA, {10, 255, 200, 0});
    Image out;
    img::load_png(out, path);
    REQUIRE(out.channels() == 2);
    REQUIRE(out.width() == 2);
    CHECK(out.get(0, 0) == 10);
    CHECK(out.get(0, 1) == 200);
    CHECK(out.get(1, 0) == 255);
    CHECK(out.get(1, 1) == 0);
}

TEST_CASE("image::load_png expands palettes unless indexing is disabled", "[image][io][png]")
{
    image_test::TempDir dir;
    std::string path = dir.file("pal.png");
    std::vector<png_color> palette = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}};
    writeRawPng(path, 3, 1, PNG_COLOR_TYPE_PALETTE, {2, 0, 1}, palette);

    Image rgb;
    img::load_png(rgb, path);
    REQUIRE(rgb.channels() == 3);
    REQUIRE(rgb.depth() == 8);
    // Pixel 0 = blue, 1 = red, 2 = green
    CHECK(rgb.get(0, 0) == 0);
    CHECK(rgb.get(2, 0) == 255);
    CHECK(rgb.get(0, 1) == 255);
    CHECK(rgb.get(1, 1) == 0);
    CHECK(rgb.get(1, 2) == 255);

    Image idx;
    img::load_png(idx, path, true);
    REQUIRE(idx.channels() == 1);
    CHECK(idx.get(0) == 2);
    CHECK(idx.get(1) == 0);
    CHECK(idx.get(2) == 1);
}

// Bug: pngio.cpp:44-50: save_png has no colour type for 2 channels, so gray+alpha
// images are written as plain grayscale (with interleaved gray/alpha bytes as
// pixel data) and the alpha channel is lost.
TEST_CASE("image::save_png round-trips gray+alpha images", "[image][io][png][!shouldfail]")
{
    image_test::TempDir dir;
    std::string path = dir.file("ga_rt.png");
    Image in = image_test::makePattern(8, 4, 3, 2);
    img::save_png(in, path);
    Image out;
    img::load_png(out, path);
    REQUIRE(out.channels() == 2);
    CHECK(image_test::samePixels(out, in));
}

TEST_CASE("image::save_png refuses empty images", "[image][io][png]")
{
    image_test::TempDir dir;
    std::string path = dir.file("empty.png");
    Image empty;
    img::save_png(empty, path);
    CHECK_FALSE(std::filesystem::exists(path));
}

TEST_CASE("image::load_png leaves the image untouched on missing or corrupt files", "[image][io][png]")
{
    image_test::TempDir dir;
    Image out(8, 2, 2, 1);
    out.fill(5);

    img::load_png(out, dir.file("missing.png"));
    CHECK(out.width() == 2);
    CHECK(out.get(0) == 5);

    // Valid signature followed by an IHDR chunk with a bad CRC and zero size
    std::vector<uint8_t> junk = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A, 0, 0, 0, 13, 'I', 'H', 'D', 'R'};
    junk.resize(junk.size() + 13 + 4, 0);
    std::string bad = dir.file("bad.png");
    image_test::writeBytes(bad, junk);
    img::load_png(out, bad);
    CHECK(out.width() == 2);
    CHECK(out.get(0) == 5);

    // Same for the buffer variant
    img::load_png(out, junk.data(), junk.size());
    CHECK(out.width() == 2);
    CHECK(out.get(0) == 5);
}
