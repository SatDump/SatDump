#include "../image_test_helpers.h"
#include "image/io.h"
#include "image/jpeg_utils.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using satdump::image::Image;
namespace img = satdump::image;

TEST_CASE("image::save_jpeg / load_jpeg round-trip within tolerance", "[image][io][jpeg]")
{
    auto channels = GENERATE(1, 3);
    image_test::TempDir dir;
    std::string path = dir.file("rt.jpg");
    Image in = image_test::makeGradient(8, 32, 24, channels);
    img::save_jpeg(in, path);

    Image out;
    img::load_jpeg(out, path);
    REQUIRE(image_test::sameShape(out, in));
    // Quality 90 on a smooth gradient
    CHECK(image_test::maxAbsDiff(out, in) <= 8);
}

TEST_CASE("image::save_jpeg stores 16-bit images as 8-bit", "[image][io][jpeg]")
{
    image_test::TempDir dir;
    std::string path = dir.file("rt16.jpg");
    Image in = image_test::makeGradient(16, 32, 24, 3);
    img::save_jpeg(in, path);

    Image out;
    img::load_jpeg(out, path);
    REQUIRE(out.depth() == 8);
    REQUIRE(out.channels() == 3);
    Image ref = in.to8bits();
    CHECK(image_test::maxAbsDiff(out, ref) <= 8);
}

TEST_CASE("image::save_jpeg premultiplies RGBA by alpha", "[image][io][jpeg]")
{
    image_test::TempDir dir;
    std::string path = dir.file("rgba.jpg");
    Image in(8, 16, 16, 4);
    for (int i = 0; i < 256; i++)
    {
        for (int c = 0; c < 3; c++)
            in.set(c, i, 200);
        in.set(3, i, (i % 16) < 8 ? 255 : 0); // left half opaque, right half transparent
    }
    img::save_jpeg(in, path);

    Image out;
    img::load_jpeg(out, path);
    REQUIRE(out.channels() == 3);
    for (int c = 0; c < 3; c++)
    {
        CHECK(std::abs(out.get(c, 2, 8) - 200) <= 10);
        CHECK(out.get(c, 13, 8) <= 10);
    }
}

TEST_CASE("image::load_jpeg from a memory buffer and via load_img", "[image][io][jpeg]")
{
    image_test::TempDir dir;
    std::string path = dir.file("buf.jpg");
    Image in = image_test::makeGradient(8, 20, 10, 3);
    img::save_jpeg(in, path);
    std::vector<uint8_t> data = image_test::readFile(path);

    Image from_file, from_buf, from_disp;
    img::load_jpeg(from_file, path);
    img::load_jpeg(from_buf, data.data(), data.size());
    img::load_img(from_disp, data.data(), data.size());
    CHECK(image_test::samePixels(from_buf, from_file));
    CHECK(image_test::samePixels(from_disp, from_file));
}

TEST_CASE("image::load_jpeg leaves the image untouched on missing or corrupt files", "[image][io][jpeg]")
{
    image_test::TempDir dir;
    Image out(8, 3, 3, 1);
    out.fill(9);

    img::load_jpeg(out, dir.file("missing.jpg"));
    CHECK(out.width() == 3);
    CHECK(out.get(0) == 9);

    std::vector<uint8_t> junk = {0xFF, 0xD8, 0xFF, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    std::string bad = dir.file("bad.jpg");
    image_test::writeFile(bad, junk);
    img::load_jpeg(out, bad);
    CHECK(out.width() == 3);
    CHECK(out.get(0) == 9);

    img::load_jpeg(out, junk.data(), junk.size());
    CHECK(out.width() == 3);
    CHECK(out.get(0) == 9);
}

TEST_CASE("image::save_jpeg refuses empty images", "[image][io][jpeg]")
{
    image_test::TempDir dir;
    Image empty;
    img::save_jpeg(empty, dir.file("empty.jpg"));
    CHECK_FALSE(std::filesystem::exists(dir.file("empty.jpg")));
}

TEST_CASE("image::save_jpeg_mem and decompress_jpeg round-trip grayscale", "[image][jpeg_utils]")
{
    auto depth = GENERATE(8, 16);
    Image in = image_test::makeGradient(depth, 24, 16, 1);
    std::vector<uint8_t> data = img::save_jpeg_mem(in);
    REQUIRE(data.size() > 4);
    CHECK(data[0] == 0xFF);
    CHECK(data[1] == 0xD8);
    CHECK(data[data.size() - 2] == 0xFF);
    CHECK(data[data.size() - 1] == 0xD9);

    Image out = img::decompress_jpeg(data.data(), data.size());
    REQUIRE(out.depth() == 8);
    REQUIRE(out.width() == 24);
    REQUIRE(out.height() == 16);
    REQUIRE(out.channels() == 1);
    Image ref = in.to8bits();
    CHECK(image_test::maxAbsDiff(out, ref) <= 8);
}

TEST_CASE("image::save_jpeg_mem output is decodable by load_jpeg for RGB", "[image][jpeg_utils]")
{
    Image in = image_test::makeGradient(8, 16, 16, 3);
    std::vector<uint8_t> data = img::save_jpeg_mem(in);
    Image out;
    img::load_jpeg(out, data.data(), data.size());
    REQUIRE(image_test::sameShape(out, in));
    CHECK(image_test::maxAbsDiff(out, in) <= 8);

    // Consecutive calls are independent
    std::vector<uint8_t> again = img::save_jpeg_mem(in);
    CHECK(again == data);
}

TEST_CASE("image::save_jpeg_mem of an empty image is empty", "[image][jpeg_utils]")
{
    Image empty;
    CHECK(img::save_jpeg_mem(empty).empty());
}

TEST_CASE("image::decompress_jpeg returns an empty image on invalid data", "[image][jpeg_utils]")
{
    std::vector<uint8_t> junk = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
    Image out = img::decompress_jpeg(junk.data(), junk.size());
    CHECK(out.width() == 0);
    CHECK(out.size() == 0);
}
