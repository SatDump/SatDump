#include "../image_test_helpers.h"
#include "image/io.h"
#include "image/j2k_utils.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using satdump::image::Image;
namespace img = satdump::image;

TEST_CASE("image::save_j2k / load_j2k round-trip losslessly", "[image][io][j2k]")
{
    auto depth = GENERATE(8, 16);
    auto channels = GENERATE(1, 3, 4);
    INFO("depth " << depth << " channels " << channels);
    image_test::TempDir dir;
    std::string path = dir.file("rt.j2k");
    Image in = image_test::makePattern(depth, 40, 24, channels);
    img::save_j2k(in, path);
    REQUIRE(std::filesystem::exists(path));

    Image out;
    img::load_j2k(out, path);
    REQUIRE(image_test::sameShape(out, in));
    CHECK(image_test::samePixels(out, in));
}

// j2kio.cpp:31,85: the encoder always uses 5 resolution levels, which OpenJPEG
// rejects for images smaller than 16 pixels in either dimension. The return
// value of opj_setup_encoder is ignored, so an empty file is written silently.
TEST_CASE("image::save_j2k / load_j2k round-trip small images", "[image][io][j2k][!shouldfail]")
{
    image_test::TempDir dir;
    std::string path = dir.file("small.j2k");
    Image in = image_test::makePattern(16, 8, 8, 1);
    img::save_j2k(in, path);
    CHECK(std::filesystem::file_size(path) > 0);
    Image out;
    img::load_j2k(out, path);
    REQUIRE(image_test::sameShape(out, in));
    CHECK(image_test::samePixels(out, in));
}

TEST_CASE("image::load_j2k leaves the image untouched on missing or corrupt files", "[image][io][j2k]")
{
    image_test::TempDir dir;
    Image out(8, 2, 2, 1);
    out.fill(7);
    img::load_j2k(out, dir.file("missing.j2k"));
    CHECK(out.width() == 2);

    std::string bad = dir.file("bad.j2k");
    image_test::writeFile(bad, {0xFF, 0x4F, 0xFF, 0x51, 0x00, 0x02, 0x00, 0x00});
    img::load_j2k(out, bad);
    CHECK(out.width() == 2);
    CHECK(out.get(0) == 7);
}

TEST_CASE("image::save_j2k refuses empty images", "[image][io][j2k]")
{
    image_test::TempDir dir;
    Image empty;
    img::save_j2k(empty, dir.file("empty.j2k"));
    CHECK_FALSE(std::filesystem::exists(dir.file("empty.j2k")));
}

TEST_CASE("image::decompress_j2k_openjp2 decodes a codestream from memory", "[image][j2k_utils]")
{
    image_test::TempDir dir;
    std::string path = dir.file("mem.j2k");
    Image in = image_test::makePattern(16, 33, 17, 1);
    img::save_j2k(in, path);
    std::vector<uint8_t> data = image_test::readFile(path);

    Image out = img::decompress_j2k_openjp2(data.data(), data.size());
    REQUIRE(out.depth() == 16);
    REQUIRE(out.channels() == 1);
    REQUIRE(out.width() == 33);
    REQUIRE(out.height() == 17);
    CHECK(image_test::samePixels(out, in));
}

TEST_CASE("image::decompress_j2k_openjp2 keeps raw sample values of 8-bit streams", "[image][j2k_utils]")
{
    // Unlike load_j2k, the buffer decoder does not rescale to 16 bits
    image_test::TempDir dir;
    std::string path = dir.file("mem8.j2k");
    Image in = image_test::makePattern(8, 16, 16, 1);
    img::save_j2k(in, path);
    std::vector<uint8_t> data = image_test::readFile(path);

    Image out = img::decompress_j2k_openjp2(data.data(), data.size());
    REQUIRE(out.depth() == 16);
    for (size_t i = 0; i < in.size(); i++)
        REQUIRE(out.get(i) == in.get(i));
}

TEST_CASE("image::decompress_j2k_openjp2 returns an empty image on invalid data", "[image][j2k_utils]")
{
    std::vector<uint8_t> junk = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    Image out = img::decompress_j2k_openjp2(junk.data(), junk.size());
    CHECK(out.width() == 0);
    CHECK(out.size() == 0);
}
