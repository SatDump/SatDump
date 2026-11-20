#include "image/meta.h"
#include "products/punctiform/product_dotmap.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdlib>

using satdump::products::PunctiformProduct;

namespace
{
    // Channel "v" with one sample per (lat, lon, value) triple, values on a 0..10 scale
    void fillProduct(PunctiformProduct &p, const std::vector<std::array<double, 3>> &samples)
    {
        PunctiformProduct::DataHolder d;
        d.channel_name = "v";
        for (auto &s : samples)
        {
            d.timestamps.push_back(0);
            d.positions.push_back(geodetic::geodetic_coords_t(s[0], s[1], 0));
            d.data.push_back(s[2]);
        }
        p.data.push_back(d);
    }

    nlohmann::json expectedProj(int w, int h)
    {
        return {{"type", "equirec"}, {"offset_x", -180.0}, {"offset_y", 90.0}, {"scalar_x", 360.0 / w}, {"scalar_y", -180.0 / h}, {"width", w}, {"height", h}};
    }
} // namespace

TEST_CASE("generate_dotmap_product_image draws scaled dots at sample positions", "[products][punctiform][dotmap]")
{
    // 0N 0E at full scale, 45N 90W at half scale
    PunctiformProduct p;
    fillProduct(p, {{0, 0, 10}, {45, -90, 5}});
    auto img = satdump::products::generate_dotmap_product_image(&p, "v", 36, 18, 2, false, false, 0, 10);

    REQUIRE(img.width() == 36);
    REQUIRE(img.height() == 18);
    REQUIRE(img.channels() == 3);
    REQUIRE(img.depth() == 8);

    // 0N 0E is pixel (18, 9); the dot has a radius of 2 pixels (the exact rim pixels are up to
    // Image::draw_circle, so only the inside is checked)
    for (int c = 0; c < 3; c++)
    {
        CHECK(img.get(c, 18, 9) == 255);
        CHECK(img.get(c, 17, 9) == 255);
        CHECK(img.get(c, 19, 9) == 255);
        CHECK(img.get(c, 18, 8) == 255);
        CHECK(img.get(c, 18, 10) == 255);
        CHECK(img.get(c, 21, 9) == 0);
        CHECK(img.get(c, 18, 12) == 0);
    }
    // 45N 90W is pixel (9, 4) (row 18 - 0.75 * 18 = 4.5 truncated)
    for (int c = 0; c < 3; c++)
        CHECK(std::abs(img.get(c, 9, 4) - 127) <= 1);
    CHECK(img.get(0, 0, 17) == 0);

    REQUIRE(satdump::image::has_metadata_proj_cfg(img));
    CHECK(satdump::image::get_metadata_proj_cfg(img) == expectedProj(36, 18));
}

TEST_CASE("generate_dotmap_product_image clamps values to the given range", "[products][punctiform][dotmap]")
{
    PunctiformProduct p;
    fillProduct(p, {{0, -90, 50}, {0, 90, -50}});
    auto img = satdump::products::generate_dotmap_product_image(&p, "v", 36, 18, 2, false, false, 0, 10);
    CHECK(img.get(0, 9, 9) == 255); // Above max => white
    CHECK(img.get(0, 27, 9) == 0);  // Below min => black
}

TEST_CASE("generate_dotmap_product_image rejects unknown channels", "[products][punctiform][dotmap]")
{
    PunctiformProduct p;
    fillProduct(p, {{0, 0, 1}});
    CHECK_THROWS(satdump::products::generate_dotmap_product_image(&p, "nope", 36, 18, 2, false, false, 0, 10));
}

TEST_CASE("generate_fillmap_product_image fills pixels near samples with the nearest value", "[products][punctiform][dotmap]")
{
    // Two samples 6 pixels apart on the equator of a 72x36 map (5 degree pixels)
    PunctiformProduct p;
    fillProduct(p, {{0, 0, 10}, {0, 30, 0}});
    float progress = 0;
    auto img = satdump::products::generate_fillmap_product_image(&p, "v", 72, 36, 0, 10, &progress);

    REQUIRE(img.width() == 72);
    REQUIRE(img.height() == 36);
    REQUIRE(img.channels() == 3);
    CHECK(progress > 0.98f);

    // Sample 1 at (36, 18), sample 2 at (42, 18)
    CHECK(img.get(0, 36, 18) == 255);
    CHECK(img.get(0, 38, 18) == 255);
    CHECK(img.get(0, 36, 22) == 255); // 4 px away: within the 5 px fill radius
    CHECK(img.get(0, 36, 23) == 0);   // 5 px away: outside
    CHECK(img.get(0, 40, 18) == 0);   // Closer to the second (black) sample
    CHECK(img.get(0, 31, 18) == 0);   // 5 px left: outside
    CHECK(img.get(0, 32, 18) == 255);

    CHECK(satdump::image::get_metadata_proj_cfg(img) == expectedProj(72, 36));
}

TEST_CASE("generate_fillmap_product_image refuses large outputs", "[products][punctiform][dotmap]")
{
    PunctiformProduct p;
    fillProduct(p, {{0, 0, 1}});
    CHECK_THROWS(satdump::products::generate_fillmap_product_image(&p, "v", 401, 100, 0, 10));
    CHECK_THROWS(satdump::products::generate_fillmap_product_image(&p, "v", 100, 401, 0, 10));
}
