#include "image/image.h"
#include "image/meta.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using satdump::image::Image;
namespace img = satdump::image;

TEST_CASE("image::meta set/get/has metadata", "[image][meta]")
{
    Image im(8, 2, 2, 1);
    CHECK_FALSE(img::has_metadata(im));
    CHECK(img::get_metadata(im).is_null());

    img::set_metadata(im, {{"key", 42}, {"name", "test"}});
    REQUIRE(img::has_metadata(im));
    auto m = img::get_metadata(im);
    CHECK(m["key"] == 42);
    CHECK(m["name"] == "test");

    // Replacing metadata
    img::set_metadata(im, {{"other", true}});
    m = img::get_metadata(im);
    CHECK_FALSE(m.contains("key"));
    CHECK(m["other"] == true);
}

TEST_CASE("image::meta metadata is deep copied with the image", "[image][meta]")
{
    Image a(8, 2, 2, 1);
    img::set_metadata(a, {{"v", 1}});

    Image b(a);
    REQUIRE(img::has_metadata(b));
    CHECK(b.metadata_obj != a.metadata_obj);
    img::set_metadata(b, {{"v", 2}});
    CHECK(img::get_metadata(a)["v"] == 1);
    CHECK(img::get_metadata(b)["v"] == 2);

    Image c(16, 1, 1, 1);
    c = a;
    REQUIRE(img::has_metadata(c));
    CHECK(img::get_metadata(c)["v"] == 1);
    CHECK(c.metadata_obj != a.metadata_obj);

    // Same-depth conversions copy metadata along with the image
    Image same = a.to8bits();
    CHECK(img::has_metadata(same));
}

// image.cpp:36 / meta.cpp:9: copy_meta only copies when the source has
// metadata, so assigning an image without metadata keeps the destination's
// stale metadata.
TEST_CASE("image::meta assigning an image without metadata clears it", "[image][meta][!shouldfail]")
{
    Image a(8, 2, 2, 1);
    img::set_metadata(a, {{"stale", true}});
    Image b(8, 2, 2, 1);
    a = b;
    CHECK_FALSE(img::has_metadata(a));
}

TEST_CASE("image::meta projection config helpers", "[image][meta]")
{
    Image im(8, 2, 2, 1);
    CHECK_FALSE(img::has_metadata_proj_cfg(im));
    CHECK(img::get_metadata_proj_cfg(im).is_null());

    img::set_metadata_proj_cfg(im, {{"type", "equirec"}, {"offset_x", -180.0}});
    REQUIRE(img::has_metadata_proj_cfg(im));
    CHECK(img::get_metadata_proj_cfg(im)["type"] == "equirec");
    CHECK(img::get_metadata_proj_cfg(im)["offset_x"] == -180.0);

    // Proj config lives inside the general metadata and does not remove other keys
    img::set_metadata(im, {{"other", 1}});
    CHECK_FALSE(img::has_metadata_proj_cfg(im));
    img::set_metadata_proj_cfg(im, {{"type", "stereo"}});
    CHECK(img::get_metadata(im)["other"] == 1);
    CHECK(img::get_metadata(im)["proj_cfg"]["type"] == "stereo");
    CHECK_FALSE(img::has_metadata_calib_cfg(im));
}

TEST_CASE("image::meta calibration config helpers", "[image][meta]")
{
    Image im(16, 2, 2, 1);
    CHECK_FALSE(img::has_metadata_calib_cfg(im));
    CHECK(img::get_metadata_calib_cfg(im).is_null());

    img::set_metadata_calib_cfg(im, {{"unit", "K"}, {"scale", 0.01}, {"offset", 100.0}});
    REQUIRE(img::has_metadata_calib_cfg(im));
    CHECK(img::get_metadata_calib_cfg(im)["unit"] == "K");
    CHECK_FALSE(img::has_metadata_proj_cfg(im));

    img::set_metadata_proj_cfg(im, {{"type", "equirec"}});
    CHECK(img::has_metadata_calib_cfg(im));
    CHECK(img::has_metadata_proj_cfg(im));
}

TEST_CASE("image::ImgCalibHandler linear conversion and JSON", "[image][meta]")
{
    img::ImgCalibHandler h;
    h.unit = "K";
    h.scale = 0.5;
    h.offset = 200;
    CHECK(h.getVal(0) == 200);
    CHECK(h.getVal(100) == 250);
    CHECK(h.setVal(250) == 100);
    CHECK(h.setVal(h.getVal(37)) == Catch::Approx(37));

    nlohmann::json j = h;
    CHECK(j["unit"] == "K");
    CHECK(j["scale"] == 0.5);
    CHECK(j["offset"] == 200.0);

    img::ImgCalibHandler back = j.get<img::ImgCalibHandler>();
    CHECK(back.unit == "K");
    CHECK(back.scale == 0.5);
    CHECK(back.offset == 200);
    // min/max are derived from offset and scale
    CHECK(back.min == 200);
    CHECK(back.max == 200.5);
}

TEST_CASE("image::ImgCalibHandler serialises min/max when scale and offset are unset", "[image][meta]")
{
    img::ImgCalibHandler h;
    h.unit = "dBZ";
    h.min = -10;
    h.max = 70;
    nlohmann::json j = h;
    CHECK(j["offset"] == -10.0);
    CHECK(j["scale"] == 80.0);

    img::ImgCalibHandler back = j.get<img::ImgCalibHandler>();
    CHECK(back.min == -10);
    CHECK(back.max == 70);

    // Missing scale/offset keys are tolerated
    img::ImgCalibHandler partial = nlohmann::json({{"unit", "x"}}).get<img::ImgCalibHandler>();
    CHECK(partial.unit == "x");
    CHECK(partial.scale == 0);
    CHECK(partial.offset == 0);
}
