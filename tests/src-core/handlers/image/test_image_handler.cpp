#include "handlers/image/image_handler.h"
#include "pipeline/pipeline_test_helpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace pipeline_test;
using satdump::handlers::ImageHandler;

namespace
{
    // 4x3 single channel image with distinct values
    satdump::image::Image smallImage()
    {
        satdump::image::Image img(8, 4, 3, 1);
        for (int y = 0; y < 3; y++)
            for (int x = 0; x < 4; x++)
                img.set(0, x, y, 10 + y * 4 + x);
        return img;
    }
} // namespace

TEST_CASE("ImageHandler names and IDs", "[handlers][image_handler]")
{
    ImageHandler h(smallImage(), "My Image/Ch 1\\x");
    CHECK(h.getID() == "image_handler");
    CHECK(h.getName() == "My Image/Ch 1\\x");
    CHECK(h.getSaneName() == "My_Image_Ch_1_x");

    h.setName("Other");
    CHECK(h.getName() == "Other");

    ImageHandler unnamed;
    CHECK(unnamed.getName() == "Unknown Image");
    CHECK(unnamed.getImage().size() == 0);
}

TEST_CASE("ImageHandler without settings returns the raw image", "[handlers][image_handler]")
{
    auto img = smallImage();
    ImageHandler h(img);

    auto &cur = h.getImage();
    REQUIRE(cur.width() == 4);
    REQUIRE(cur.height() == 3);
    for (size_t i = 0; i < img.size(); i++)
        CHECK(cur.get(i) == img.get(i));
    CHECK(&h.getImage(true) == &h.getImage(false));
}

TEST_CASE("ImageHandler::getConfig default values", "[handlers][image_handler]")
{
    ImageHandler h(smallImage());
    auto cfg = h.getConfig();
    CHECK(cfg["rotate"] == 0);
    CHECK(cfg["geocorrect"] == false);
    CHECK(cfg["filters"].is_array());
    CHECK(cfg["filters"].empty());
}

TEST_CASE("ImageHandler applies configured filters on process", "[handlers][image_handler]")
{
    auto img = smallImage();
    ImageHandler h(img);

    h.setConfig({{"filters", nlohmann::json::array({nlohmann::json::array({"invert", nlohmann::json::object()})})}});
    auto cfg = h.getConfig();
    REQUIRE(cfg["filters"].size() == 1);
    CHECK(cfg["filters"][0][0] == "invert");

    h.process();

    auto &cur = h.getImage();
    auto &raw = h.getImage(false);
    REQUIRE(cur.size() == img.size());
    for (size_t i = 0; i < img.size(); i++)
    {
        CHECK(cur.get(i) == 255 - img.get(i));
        CHECK(raw.get(i) == img.get(i));
    }

    // Applying the same filter twice restores the original
    h.setConfig({{"filters", nlohmann::json::array({nlohmann::json::array({"invert", nlohmann::json::object()}), nlohmann::json::array({"invert", nlohmann::json::object()})})}});
    h.process();
    for (size_t i = 0; i < img.size(); i++)
        CHECK(h.getImage().get(i) == img.get(i));

    // Clearing the filters goes back to the raw image
    h.setConfig(nlohmann::json::object());
    h.process();
    CHECK(h.getConfig()["filters"].empty());
    CHECK(&h.getImage() == &h.getImage(false));
}

TEST_CASE("ImageHandler skips unknown filters", "[handlers][image_handler]")
{
    auto img = smallImage();
    ImageHandler h(img);
    h.setConfig({{"filters", nlohmann::json::array({nlohmann::json::array({"unit_no_such_filter", nlohmann::json::object()})})}});
    h.process();

    for (size_t i = 0; i < img.size(); i++)
        CHECK(h.getImage().get(i) == img.get(i));
}

TEST_CASE("ImageHandler rotates the processed image by 180 degrees", "[handlers][image_handler]")
{
    auto img = smallImage();
    ImageHandler h(img);

    // Note: setConfig reads the rotation from "rotate180", unlike getConfig which writes "rotate"
    h.setConfig({{"rotate180", 180}});
    CHECK(h.getConfig()["rotate"] == 180);
    h.process();

    auto &cur = h.getImage();
    REQUIRE(cur.width() == 4);
    REQUIRE(cur.height() == 3);
    for (int y = 0; y < 3; y++)
        for (int x = 0; x < 4; x++)
            CHECK(cur.get(0, x, y) == img.get(0, 3 - x, 2 - y));
}

// Bug: image_handler.cpp:518 : setConfig() reads the rotation from p["rotate180"] while getConfig()
// (image_handler.cpp:537) stores it as "rotate", so a saved configuration loses its rotation.
TEST_CASE("ImageHandler::setConfig accepts what getConfig returns", "[handlers][image_handler][!shouldfail]")
{
    ImageHandler a(smallImage());
    a.setConfig({{"rotate180", 90}});
    auto cfg = a.getConfig();
    REQUIRE(cfg["rotate"] == 90);

    ImageHandler b(smallImage());
    b.setConfig(cfg);
    CHECK(b.getConfig()["rotate"] == 90);
}

TEST_CASE("ImageHandler::resetConfig clears images and settings", "[handlers][image_handler]")
{
    ImageHandler h(smallImage());
    h.setConfig({{"rotate180", 180}, {"filters", nlohmann::json::array({nlohmann::json::array({"invert", nlohmann::json::object()})})}});
    h.resetConfig();

    auto cfg = h.getConfig();
    CHECK(cfg["rotate"] == 0);
    CHECK(cfg["filters"].empty());
    CHECK(h.getImage(false).size() == 0);
}

TEST_CASE("ImageHandler::setImage replaces the raw image and reprocesses", "[handlers][image_handler]")
{
    ImageHandler h(smallImage());
    h.setConfig({{"filters", nlohmann::json::array({nlohmann::json::array({"invert", nlohmann::json::object()})})}});

    satdump::image::Image other(8, 2, 2, 1);
    other.fill(100);
    h.setImage(other);

    REQUIRE(h.getImage().width() == 2);
    for (size_t i = 0; i < 4; i++)
        CHECK(h.getImage().get(i) == 155);
    CHECK(h.getImage(false).get(0) == 100);
}

TEST_CASE("ImageHandler::saveResult writes the current image", "[handlers][image_handler]")
{
    TempDir tmp("handlers_img");
    ImageHandler h(smallImage(), "saved image.png");

    CHECK(h.saveResult(tmp.str()));
    CHECK(std::filesystem::exists(tmp.file("saved_image.png")));

    ImageHandler empty(satdump::image::Image(), "empty.png");
    CHECK_FALSE(empty.saveResult(tmp.str()));
}
