#include "image/image_test_helpers.h"
#include "image/io.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using satdump::image::Image;
namespace img = satdump::image;

TEST_CASE("image::save_qoi / load_qoi round-trip 8-bit exactly", "[image][io][qoi]")
{
    auto channels = GENERATE(1, 3, 4);
    INFO("channels " << channels);
    image_test::TempDir dir;
    std::string path = dir.file("rt.qoi");
    Image in = image_test::makePattern(8, 12, 9, channels);
    img::save_qoi(in, path);

    Image out;
    img::load_qoi(out, path);
    REQUIRE(image_test::sameShape(out, in));
    CHECK(image_test::samePixels(out, in));
}

TEST_CASE("image::save_qoi stores 16-bit images as 8-bit", "[image][io][qoi]")
{
    auto channels = GENERATE(1, 3, 4);
    image_test::TempDir dir;
    std::string path = dir.file("rt16.qoi");
    Image in = image_test::makePattern(16, 7, 5, channels);
    img::save_qoi(in, path);

    Image out;
    img::load_qoi(out, path);
    REQUIRE(out.depth() == 8);
    REQUIRE(out.channels() == channels);
    Image ref = in.to8bits();
    CHECK(image_test::samePixels(out, ref));
}

TEST_CASE("image::save_qoi marks single-channel files with a trailing byte", "[image][io][qoi]")
{
    image_test::TempDir dir;
    Image gray(8, 2, 2, 1), rgb(8, 2, 2, 3);
    img::save_qoi(gray, dir.file("g.qoi"));
    img::save_qoi(rgb, dir.file("c.qoi"));
    auto g = image_test::readBytes(dir.file("g.qoi"));
    auto c = image_test::readBytes(dir.file("c.qoi"));
    REQUIRE(g.size() >= 4);
    CHECK(std::string(g.begin(), g.begin() + 4) == "qoif");
    CHECK(g.back() == 0xAA);
    CHECK(c.back() != 0xAA);
    CHECK(g.size() == c.size() + 1);
}

TEST_CASE("image::load_qoi leaves the image untouched on a missing file", "[image][io][qoi]")
{
    image_test::TempDir dir;
    Image out(8, 2, 2, 1);
    out.fill(1);
    img::load_qoi(out, dir.file("missing.qoi"));
    CHECK(out.width() == 2);
    CHECK(out.get(0) == 1);
}

TEST_CASE("image::save_qoi refuses empty images", "[image][io][qoi]")
{
    image_test::TempDir dir;
    Image empty;
    img::save_qoi(empty, dir.file("empty.qoi"));
    CHECK_FALSE(std::filesystem::exists(dir.file("empty.qoi")));
}
