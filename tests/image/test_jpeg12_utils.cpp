#include "image/image_test_helpers.h"
#include "image/jpeg12_utils.h"
#include <catch2/catch_test_macros.hpp>
#include <csetjmp>
#include <cstdio>
extern "C"
{
#include "libs/jpeg12/jpeglib12.h"
}

using satdump::image::Image;

namespace
{
    struct Jpeg12Error
    {
        jpeg_error_mgr pub;
        jmp_buf jmp;
    };

    void jpeg12ErrorExit(j_common_ptr cinfo) { longjmp(((Jpeg12Error *)cinfo->err)->jmp, 1); }

    // Encodes 12-bit grayscale samples with the bundled 12-bit libjpeg.
    // Returns an empty vector if encoding failed.
    std::vector<uint8_t> encodeJpeg12(const std::vector<short> &samples, int w, int h, bool lossless)
    {
        image_test::TempDir dir;
        std::string path = dir.file("enc12.jpg");
        FILE *fp = fopen(path.c_str(), "wb");
        if (fp == nullptr)
            return {};

        jpeg_compress_struct cinfo;
        Jpeg12Error jerr;
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = jpeg12ErrorExit;
        if (setjmp(jerr.jmp))
        {
            jpeg_destroy_compress(&cinfo);
            fclose(fp);
            return {};
        }

        jpeg_create_compress(&cinfo);
        jpeg_stdio_dest(&cinfo, fp);
        cinfo.image_width = w;
        cinfo.image_height = h;
        cinfo.input_components = 1;
        cinfo.in_color_space = JCS_GRAYSCALE;
        jpeg_set_defaults(&cinfo);
        if (lossless)
            jpeg_simple_lossless(&cinfo, 1, 0);
        else
            jpeg_set_quality(&cinfo, 95, TRUE);
        jpeg_start_compress(&cinfo, TRUE);
        std::vector<short> row(w);
        while (cinfo.next_scanline < cinfo.image_height)
        {
            for (int x = 0; x < w; x++)
                row[x] = samples[cinfo.next_scanline * w + x];
            JSAMPROW rows[1] = {(JSAMPROW)row.data()};
            jpeg_write_scanlines(&cinfo, rows, 1);
        }
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        fclose(fp);
        return image_test::readBytes(path);
    }

    std::vector<short> makeSamples12(int w, int h)
    {
        std::vector<short> s(w * h);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                s[y * w + x] = (x * 4095 / (w - 1) + y * 37) % 4096;
        return s;
    }
} // namespace

TEST_CASE("image::decompress_jpeg12 decodes lossless 12-bit data scaled to 16 bits", "[image][jpeg12_utils]")
{
    const int w = 24, h = 16;
    auto samples = makeSamples12(w, h);
    auto data = encodeJpeg12(samples, w, h, true);
    REQUIRE(!data.empty());

    Image out = satdump::image::decompress_jpeg12(data.data(), data.size());
    REQUIRE(out.depth() == 16);
    REQUIRE(out.channels() == 1);
    REQUIRE(out.width() == size_t(w));
    REQUIRE(out.height() == size_t(h));
    for (int i = 0; i < w * h; i++)
        REQUIRE(out.get(i) == samples[i] << 4);
}

TEST_CASE("image::decompress_jpeg12 decodes lossy 12-bit data within tolerance", "[image][jpeg12_utils]")
{
    const int w = 32, h = 16;
    std::vector<short> samples(w * h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            samples[y * w + x] = x * 100 + y * 20; // smooth ramp
    auto data = encodeJpeg12(samples, w, h, false);
    REQUIRE(!data.empty());

    Image out = satdump::image::decompress_jpeg12(data.data(), data.size());
    REQUIRE(out.width() == size_t(w));
    REQUIRE(out.height() == size_t(h));
    for (int i = 0; i < w * h; i++)
        REQUIRE(std::abs((out.get(i) >> 4) - samples[i]) <= 40);
}

TEST_CASE("image::decompress_jpeg12 returns an empty image on invalid data", "[image][jpeg12_utils]")
{
    std::vector<uint8_t> junk = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
    Image out = satdump::image::decompress_jpeg12(junk.data(), junk.size());
    CHECK(out.width() == 0);
    CHECK(out.size() == 0);
}
