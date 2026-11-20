#include "handlers/image/image_filter.h"
#include "image/simple_mean_filter.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <set>

namespace
{
    using satdump::handlers::getImageFilters;

    satdump::image::Image gradient(int w, int h, int channels = 1)
    {
        satdump::image::Image img(8, w, h, channels);
        for (int c = 0; c < channels; c++)
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                    img.set(c, x, y, (x * 16 + y * 4 + c * 50) % 256);
        return img;
    }
} // namespace

TEST_CASE("getImageFilters lists all built-in filters", "[handlers][image_filter]")
{
    auto filters = getImageFilters();

    std::set<std::string> keys;
    for (auto &f : filters)
        keys.insert(f.first);

    CHECK(keys == std::set<std::string>{"equalize", "normalize", "white_balance", "invert", "median_blur", "kuwahara_filter", "brightness_contrast", "remove_background", "hue_saturation",
                                        "adaptive_despeckle", "simple_mean_filter"});

    for (auto &f : filters)
    {
        INFO(f.first);
        CHECK_FALSE(f.second.name.empty());
        CHECK(f.second.perform != nullptr);

        // Filters with a menu provide a configurator, the others do not
        auto cfg = f.second.configMenuGetter();
        CHECK((cfg != nullptr) == f.second.has_menu);
    }

    CHECK(filters["invert"].name == "Invert");
    CHECK(filters["brightness_contrast"].has_menu);
    CHECK_FALSE(filters["normalize"].has_menu);
}

TEST_CASE("getImageFilters configurators have JSON defaults their filter accepts", "[handlers][image_filter]")
{
    auto filters = getImageFilters();

    for (auto &f : filters)
    {
        if (!f.second.has_menu)
            continue;
        INFO(f.first);
        auto cfg = f.second.configMenuGetter();
        nlohmann::json defaults = cfg->get();
        CHECK(defaults.is_object());

        // Applying a filter with its configurator defaults must work on a small RGB image
        auto img = gradient(16, 8, 3);
        float progress = 0;
        CHECK_NOTHROW(f.second.perform(img, defaults, &progress));
        CHECK(img.width() == 16);
        CHECK(img.height() == 8);
    }
}

TEST_CASE("getImageFilters configurator get/set round-trips", "[handlers][image_filter]")
{
    auto filters = getImageFilters();

    SECTION("equalize")
    {
        auto cfg = filters["equalize"].configMenuGetter();
        CHECK(cfg->get() == nlohmann::json{{"per_channel", false}});
        cfg->set({{"per_channel", true}});
        CHECK(cfg->get() == nlohmann::json{{"per_channel", true}});
    }

    SECTION("brightness_contrast")
    {
        auto cfg = filters["brightness_contrast"].configMenuGetter();
        CHECK(cfg->get()["brightness"] == 0.0);
        CHECK(cfg->get()["contrast"] == 0.0);
        cfg->set({{"brightness", 0.5}, {"contrast", -0.25}});
        CHECK(cfg->get()["brightness"].get<float>() == Catch::Approx(0.5));
        CHECK(cfg->get()["contrast"].get<float>() == Catch::Approx(-0.25));
    }

    SECTION("simple_mean_filter")
    {
        auto cfg = filters["simple_mean_filter"].configMenuGetter();
        auto def = cfg->get();
        CHECK(def["radius"] == 3);
        CHECK(def["mode"] == (int)satdump::image::SHARPEN);
        cfg->set({{"mode", (int)satdump::image::EDGE}, {"radius", 5}});
        CHECK(cfg->get()["radius"] == 5);
        CHECK(cfg->get()["mode"] == (int)satdump::image::EDGE);
    }

    SECTION("adaptive_despeckle")
    {
        auto cfg = filters["adaptive_despeckle"].configMenuGetter();
        auto def = cfg->get();
        CHECK(def["radius"] == 3);
        CHECK(def["black_level"] == 0);
        CHECK(def["white_level"] == 255);
        def["radius"] = 7;
        def["white_level"] = 200;
        cfg->set(def);
        CHECK(cfg->get() == def);
    }

    SECTION("hue_saturation")
    {
        auto cfg = filters["hue_saturation"].configMenuGetter();
        auto def = cfg->get();
        REQUIRE(def["hue"].size() == 7);
        REQUIRE(def["saturation"].size() == 7);
        REQUIRE(def["lightness"].size() == 7);
        CHECK(def.contains("overlap"));
        def["hue"][1] = 0.25;
        cfg->set(def);
        CHECK(cfg->get()["hue"][1] == 0.25);
    }

    SECTION("missing keys throw")
    {
        auto cfg = filters["brightness_contrast"].configMenuGetter();
        CHECK_THROWS(cfg->set(nlohmann::json::object()));
    }
}

TEST_CASE("getImageFilters invert filter inverts every sample", "[handlers][image_filter]")
{
    auto filters = getImageFilters();
    auto img = gradient(8, 4, 3);
    auto orig = img;

    filters["invert"].perform(img, {}, nullptr);

    for (size_t i = 0; i < img.size(); i++)
        CHECK(img.get(i) == 255 - orig.get(i));
}

TEST_CASE("getImageFilters normalize filter stretches to the full range", "[handlers][image_filter]")
{
    auto filters = getImageFilters();
    satdump::image::Image img(8, 4, 1, 1);
    img.set(0, 50);
    img.set(1, 75);
    img.set(2, 100);
    img.set(3, 60);

    filters["normalize"].perform(img, {}, nullptr);

    CHECK(img.get(0) == 0);
    CHECK(img.get(2) == 255);
    CHECK(img.get(1) > img.get(3));
}

TEST_CASE("getImageFilters brightness_contrast with zero settings is the identity", "[handlers][image_filter]")
{
    auto filters = getImageFilters();
    auto img = gradient(8, 8);
    auto orig = img;
    float progress = 0;

    filters["brightness_contrast"].perform(img, {{"brightness", 0.0}, {"contrast", 0.0}}, &progress);

    for (size_t i = 0; i < img.size(); i++)
        CHECK(std::abs(img.get(i) - orig.get(i)) <= 1);
}

TEST_CASE("getImageFilters equalize filter requires its configuration", "[handlers][image_filter]")
{
    auto filters = getImageFilters();
    auto img = gradient(4, 4);
    CHECK_THROWS(filters["equalize"].perform(img, nlohmann::json::object(), nullptr));
}
