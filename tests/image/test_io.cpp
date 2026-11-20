#include "image/io.h"
#include "image_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using satdump::image::Image;
namespace img = satdump::image;

TEST_CASE("image::save_img / load_img dispatch on extension and signature", "[image][io]")
{
    // Lossless formats only, so pixels can be compared exactly
    auto ext = GENERATE(std::string(".png"), std::string(".tif"), std::string(".tiff"), std::string(".gtif"), std::string(".j2k"), std::string(".pgm"));
    auto depth = GENERATE(8, 16);
    INFO("ext " << ext << " depth " << depth);
    image_test::TempDir dir;
    std::string path = dir.file("dispatch" + ext);
    Image in = image_test::makePattern(depth, 20, 18, 1);
    img::save_img(in, path);
    REQUIRE(std::filesystem::exists(path));

    Image out;
    img::load_img(out, path);
    REQUIRE(image_test::sameShape(out, in));
    CHECK(image_test::samePixels(out, in));
}

TEST_CASE("image::save_img / load_img dispatch for RGB formats", "[image][io]")
{
    auto ext = GENERATE(std::string(".ppm"), std::string(".qoi"), std::string(".png"));
    image_test::TempDir dir;
    std::string path = dir.file("rgb" + ext);
    Image in = image_test::makePattern(8, 9, 7, 3);
    img::save_img(in, path);
    Image out;
    img::load_img(out, path);
    REQUIRE(image_test::sameShape(out, in));
    CHECK(image_test::samePixels(out, in));
}

TEST_CASE("image::save_img / load_img dispatch for JPEG", "[image][io]")
{
    auto ext = GENERATE(std::string(".jpg"), std::string(".jpeg"));
    image_test::TempDir dir;
    std::string path = dir.file("photo" + ext);
    Image in = image_test::makeGradient(8, 16, 16, 3);
    img::save_img(in, path);
    Image out;
    img::load_img(out, path);
    REQUIRE(image_test::sameShape(out, in));
    CHECK(image_test::maxAbsDiff(out, in) <= 8);
}

TEST_CASE("image::load_img ignores files with an unknown signature", "[image][io]")
{
    image_test::TempDir dir;
    std::string path = dir.file("text.bin");
    std::string text = "just some text, not an image";
    image_test::writeFile(path, std::vector<uint8_t>(text.begin(), text.end()));
    Image out(8, 2, 2, 1);
    out.fill(1);
    img::load_img(out, path);
    CHECK(out.width() == 2);
    CHECK(out.get(0) == 1);

    // Buffer variant only knows PNG and JPEG
    Image out2(8, 2, 2, 1);
    std::vector<uint8_t> buf(text.begin(), text.end());
    img::load_img(out2, buf.data(), buf.size());
    CHECK(out2.width() == 2);
}

TEST_CASE("image::append_ext keeps known extensions", "[image][io]")
{
    for (std::string name : {"a.png", "b.jpeg", "c.jpg", "d.j2k", "e.pgm", "f.pbm", "g.ppm", "h.tif", "i.tiff", "j.gtif", "k.qoi"})
    {
        std::string file = "/some/dir/" + name;
        std::string orig = file;
        CHECK(img::append_ext(&file));
        CHECK(file == orig);
    }
}

TEST_CASE("image::append_ext uses the configured default format", "[image][io]")
{
    image_test::ConfigGuard guard;
    satdump::satdump_cfg.main_cfg = nlohmann::ordered_json::object();
    satdump::satdump_cfg.main_cfg["satdump_general"]["image_format"]["value"] = "qoi";
    satdump::satdump_cfg.main_cfg["satdump_general"]["product_format"]["value"] = "tif";

    std::string file = "/tmp/name";
    CHECK(img::append_ext(&file));
    CHECK(file == "/tmp/name.qoi");

    std::string prod = "/tmp/product";
    CHECK(img::append_ext(&prod, true));
    CHECK(prod == "/tmp/product.tif");
}

TEST_CASE("image::append_ext fails without a valid configured format", "[image][io]")
{
    image_test::ConfigGuard guard;

    SECTION("no config")
    {
        satdump::satdump_cfg.main_cfg = nlohmann::ordered_json::object();
        std::string file = "/tmp/name";
        CHECK_FALSE(img::append_ext(&file));
        CHECK(file == "/tmp/name");
    }

    SECTION("unsupported format")
    {
        satdump::satdump_cfg.main_cfg = nlohmann::ordered_json::object();
        satdump::satdump_cfg.main_cfg["satdump_general"]["image_format"]["value"] = "bmp";
        std::string file = "/tmp/name";
        CHECK_FALSE(img::append_ext(&file));
        CHECK(file == "/tmp/name");
    }
}

TEST_CASE("image::save_img without extension and config writes nothing", "[image][io]")
{
    image_test::ConfigGuard guard;
    satdump::satdump_cfg.main_cfg = nlohmann::ordered_json::object();
    image_test::TempDir dir;
    Image in(8, 2, 2, 1);
    img::save_img(in, dir.file("noext"));
    CHECK(std::filesystem::is_empty(dir.path));
}

TEST_CASE("image::save_img appends the configured extension", "[image][io]")
{
    image_test::ConfigGuard guard;
    satdump::satdump_cfg.main_cfg = nlohmann::ordered_json::object();
    satdump::satdump_cfg.main_cfg["satdump_general"]["image_format"]["value"] = "png";
    image_test::TempDir dir;
    Image in = image_test::makePattern(8, 3, 3, 1);
    img::save_img(in, dir.file("auto"));
    REQUIRE(std::filesystem::exists(dir.file("auto.png")));
    Image out;
    img::load_img(out, dir.file("auto.png"));
    CHECK(image_test::samePixels(out, in));
}

TEST_CASE("image::save_img_safe never overwrites existing files", "[image][io]")
{
    image_test::ConfigGuard guard;
    satdump::satdump_cfg.main_cfg = nlohmann::ordered_json::object();
    satdump::satdump_cfg.main_cfg["satdump_general"]["image_format"]["value"] = "png";
    image_test::TempDir dir;

    Image first(8, 2, 2, 1), second(8, 2, 2, 1), third(8, 2, 2, 1);
    first.fill(1);
    second.fill(2);
    third.fill(3);
    img::save_img_safe(first, dir.file("img"));
    img::save_img_safe(second, dir.file("img"));
    img::save_img_safe(third, dir.file("img"));

    Image a, b, c;
    img::load_img(a, dir.file("img.png"));
    img::load_img(b, dir.file("img_2.png"));
    img::load_img(c, dir.file("img_3.png"));
    REQUIRE(a.size() == 4);
    REQUIRE(b.size() == 4);
    REQUIRE(c.size() == 4);
    CHECK(a.get(0) == 1);
    CHECK(b.get(0) == 2);
    CHECK(c.get(0) == 3);
}
