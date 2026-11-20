#include "../image_test_helpers.h"
#include "image/io.h"
#include "image/meta.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <tiffio.h>

using satdump::image::Image;
namespace img = satdump::image;

TEST_CASE("image::save_tiff / load_tiff round-trip exactly", "[image][io][tiff]")
{
    auto depth = GENERATE(8, 16);
    auto channels = GENERATE(1, 3, 4);
    INFO("depth " << depth << " channels " << channels);
    image_test::TempDir dir;
    std::string path = dir.file("rt.tif");
    Image in = image_test::makePattern(depth, 11, 6, channels);
    img::save_tiff(in, path);

    Image out;
    img::load_tiff(out, path);
    REQUIRE(image_test::sameShape(out, in));
    CHECK(image_test::samePixels(out, in));
    // No projection metadata was written
    CHECK_FALSE(img::has_metadata_proj_cfg(out));
}

TEST_CASE("image::load_tiff reads tiled TIFFs", "[image][io][tiff]")
{
    // Tiled 8-bit RGB file written directly with libtiff, 20x20 with 16x16 tiles
    image_test::TempDir dir;
    std::string path = dir.file("tiled.tif");
    const int W = 20, H = 20, T = 16;
    Image ref = image_test::makePattern(8, W, H, 3);
    {
        TIFF *tif = TIFFOpen(path.c_str(), "w");
        REQUIRE(tif != nullptr);
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, W);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, H);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
        TIFFSetField(tif, TIFFTAG_TILEWIDTH, T);
        TIFFSetField(tif, TIFFTAG_TILELENGTH, T);
        std::vector<uint8_t> tile(T * T * 3);
        for (int ty = 0; ty < H; ty += T)
            for (int tx = 0; tx < W; tx += T)
            {
                std::fill(tile.begin(), tile.end(), 0);
                for (int y = 0; y < T; y++)
                    for (int x = 0; x < T; x++)
                        if (tx + x < W && ty + y < H)
                            for (int c = 0; c < 3; c++)
                                tile[(y * T + x) * 3 + c] = ref.get(c, tx + x, ty + y);
                TIFFWriteTile(tif, tile.data(), tx, ty, 0, 0);
            }
        TIFFClose(tif);
    }

    Image out;
    img::load_tiff(out, path);
    REQUIRE(image_test::sameShape(out, ref));
    CHECK(image_test::samePixels(out, ref));
}

TEST_CASE("image::load_tiff rejects unsupported bit depths", "[image][io][tiff]")
{
    image_test::TempDir dir;
    std::string path = dir.file("float.tif");
    {
        TIFF *tif = TIFFOpen(path.c_str(), "w");
        REQUIRE(tif != nullptr);
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, 2);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, 1);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 32);
        TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        float row[2] = {0.5f, 1.0f};
        TIFFWriteScanline(tif, row, 0, 0);
        TIFFClose(tif);
    }
    Image out(8, 3, 3, 1);
    out.fill(4);
    img::load_tiff(out, path);
    CHECK(out.width() == 3);
    CHECK(out.get(0) == 4);
}

TEST_CASE("image::load_tiff leaves the image untouched on missing or corrupt files", "[image][io][tiff]")
{
    image_test::TempDir dir;
    Image out(8, 3, 3, 1);
    out.fill(6);
    img::load_tiff(out, dir.file("missing.tif"));
    CHECK(out.width() == 3);

    std::string bad = dir.file("bad.tif");
    image_test::writeFile(bad, {'I', 'I', '*', 0, 0xFF, 0xFF, 0xFF, 0x7F, 1, 2, 3});
    img::load_tiff(out, bad);
    CHECK(out.width() == 3);
    CHECK(out.get(0) == 6);
}

TEST_CASE("image::save_tiff refuses empty images", "[image][io][tiff]")
{
    image_test::TempDir dir;
    Image empty;
    img::save_tiff(empty, dir.file("empty.tif"));
    CHECK_FALSE(std::filesystem::exists(dir.file("empty.tif")));
}
