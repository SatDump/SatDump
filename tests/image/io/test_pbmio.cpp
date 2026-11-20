#include "../image_test_helpers.h"
#include "image/io.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <string>

using satdump::image::Image;
namespace img = satdump::image;

TEST_CASE("image::save_pbm / load_pbm round-trip gray and RGB exactly", "[image][io][pbm]")
{
    auto depth = GENERATE(8, 16);
    auto channels = GENERATE(1, 3);
    INFO("depth " << depth << " channels " << channels);
    image_test::TempDir dir;
    std::string path = dir.file(channels == 1 ? "rt.pgm" : "rt.ppm");
    Image in = image_test::makePattern(depth, 10, 7, channels);
    img::save_pbm(in, path);

    Image out;
    img::load_pbm(out, path);
    REQUIRE(image_test::sameShape(out, in));
    CHECK(image_test::samePixels(out, in));
}

TEST_CASE("image::save_pbm writes a standard binary header", "[image][io][pbm]")
{
    image_test::TempDir dir;
    std::string path = dir.file("hdr.pgm");
    Image in(8, 3, 2, 1);
    for (int i = 0; i < 6; i++)
        in.set(i, i * 40);
    img::save_pbm(in, path);
    std::vector<uint8_t> data = image_test::readFile(path);
    std::string header = "P5 3 2 255\n";
    REQUIRE(data.size() == header.size() + 6);
    CHECK(std::string(data.begin(), data.begin() + header.size()) == header);
    for (int i = 0; i < 6; i++)
        CHECK(data[header.size() + i] == i * 40);

    // 16-bit samples are big endian
    Image in16(16, 1, 1, 1);
    in16.set(0, 0x1234);
    std::string path16 = dir.file("hdr16.pgm");
    img::save_pbm(in16, path16);
    std::vector<uint8_t> d16 = image_test::readFile(path16);
    std::string header16 = "P5 1 1 65535\n";
    REQUIRE(d16.size() == header16.size() + 2);
    CHECK(d16[header16.size()] == 0x12);
    CHECK(d16[header16.size() + 1] == 0x34);
}

TEST_CASE("image::save_pbm flattens RGBA onto black", "[image][io][pbm]")
{
    auto depth = GENERATE(8, 16);
    image_test::TempDir dir;
    std::string path = dir.file("rgba.ppm");
    Image in(depth, 3, 1, 4);
    int m = in.maxval();
    for (int c = 0; c < 3; c++)
        for (int i = 0; i < 3; i++)
            in.set(c, i, m);
    in.set(3, 0, m);
    in.set(3, 1, 0);
    in.set(3, 2, m / 2);
    img::save_pbm(in, path);

    Image out;
    img::load_pbm(out, path);
    REQUIRE(out.channels() == 3);
    for (int c = 0; c < 3; c++)
    {
        CHECK(out.get(c, 0) == m);
        CHECK(out.get(c, 1) == 0);
        CHECK(std::abs(out.get(c, 2) - m / 2) <= 1);
    }
}

TEST_CASE("image::save_pbm writes only the first channel of gray+alpha", "[image][io][pbm]")
{
    image_test::TempDir dir;
    std::string path = dir.file("ga.pgm");
    Image in = image_test::makePattern(8, 4, 4, 2);
    img::save_pbm(in, path);
    Image out;
    img::load_pbm(out, path);
    REQUIRE(out.channels() == 1);
    for (int i = 0; i < 16; i++)
        REQUIRE(out.get(i) == in.get(0, i));
}

TEST_CASE("image::load_pbm leaves the image untouched on missing or invalid files", "[image][io][pbm]")
{
    image_test::TempDir dir;
    Image out(8, 2, 2, 1);
    out.fill(3);

    img::load_pbm(out, dir.file("missing.pgm"));
    CHECK(out.width() == 2);

    std::string bad = dir.file("bad.pgm");
    std::string text = "P3 2 2 255\n1 2 3 4\n"; // ASCII format, unsupported
    image_test::writeFile(bad, std::vector<uint8_t>(text.begin(), text.end()));
    img::load_pbm(out, bad);
    CHECK(out.width() == 2);
    CHECK(out.get(0) == 3);
}

TEST_CASE("image::save_pbm refuses empty images", "[image][io][pbm]")
{
    image_test::TempDir dir;
    Image empty;
    img::save_pbm(empty, dir.file("empty.pgm"));
    CHECK_FALSE(std::filesystem::exists(dir.file("empty.pgm")));
}
