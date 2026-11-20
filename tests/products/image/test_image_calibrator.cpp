#include "image/meta.h"
#include "products/image/image_calibrator.h"
#include "../products_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using satdump::products::ImageCalibrator;
using satdump::products::ImageProduct;
using satdump_test_products::EventBusGuard;

namespace
{
    // calibrated = gain * raw + 1000 * abs_index, gain read from the product calibration config
    class LinearTestCalibrator : public ImageCalibrator
    {
        double gain;

    public:
        LinearTestCalibrator(ImageProduct *p, nlohmann::json c) : ImageCalibrator(p, c), gain(c["gain"].get<double>()) {}

    protected:
        double compute(int abs_idx, int, int, uint32_t val) override { return gain * val + 1000.0 * abs_idx; }
    };

    void registerTestCalibrator()
    {
        satdump::eventBus->register_handler<satdump::products::RequestImageCalibratorEvent>(
            [](const satdump::products::RequestImageCalibratorEvent &evt)
            {
                if (evt.id == "unit_test_linear")
                    evt.calibrators.push_back(std::make_shared<LinearTestCalibrator>(evt.products, evt.calib));
            });
    }

    // Channel "ch<abs>" filled with raw = 10 * x + y
    void addChannel(ImageProduct &p, int abs_index, int depth, int w, int h, std::string unit = "")
    {
        ImageProduct::ImageHolder hd;
        hd.abs_index = abs_index;
        hd.channel_name = "ch" + std::to_string(abs_index);
        hd.image = satdump::image::Image(depth, w, h, 1);
        hd.bit_depth = depth;
        hd.calibration_type = unit;
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                hd.image.set(0, x, y, 10 * x + y);
        p.images.push_back(hd);
    }
} // namespace

TEST_CASE("get_calibrator_from_product falls back to raw values without calibration", "[products][image_calibrator]")
{
    ImageProduct p;
    addChannel(p, 4, 16, 5, 3);
    p.images[0].bit_depth = 14; // Raw counts are rescaled to the instrument depth
    auto cal = satdump::products::get_calibrator_from_product(&p);
    REQUIRE(cal);
    CHECK(cal->compute(0, 3, 2) == (32 >> 2));
    CHECK(cal->compute(0, 4, 0) == (40 >> 2));
    CHECK(cal->compute(0, 5, 0) == CALIBRATION_INVALID_VALUE);  // Out of the image
    CHECK(cal->compute(0, -1, 0) == CALIBRATION_INVALID_VALUE);
    CHECK(cal->compute(0, 0, 3) == CALIBRATION_INVALID_VALUE);
}

TEST_CASE("get_calibrator_from_product throws for an unknown calibrator", "[products][image_calibrator]")
{
    EventBusGuard bus;
    ImageProduct p;
    addChannel(p, 0, 8, 2, 2);
    p.set_calibration("no_such_calibrator", {});
    CHECK_THROWS(satdump::products::get_calibrator_from_product(&p));
}

TEST_CASE("get_calibrator_from_product uses calibrators registered through the event bus", "[products][image_calibrator]")
{
    EventBusGuard bus;
    registerTestCalibrator();

    ImageProduct p;
    addChannel(p, 0, 8, 4, 4);
    addChannel(p, 3, 8, 4, 4);
    p.set_calibration("unit_test_linear", {{"gain", 0.5}});
    auto cal = satdump::products::get_calibrator_from_product(&p);
    REQUIRE(cal);
    // The vector index is translated to the channel's absolute index
    CHECK(cal->compute(0, 2, 3) == 0.5 * 23);
    CHECK(cal->compute(1, 2, 3) == 3000 + 0.5 * 23);
    CHECK(cal->compute(1, 4, 3) == CALIBRATION_INVALID_VALUE);
}

TEST_CASE("generate_calibrated_product_channel maps calibrated values into the requested range", "[products][image_calibrator]")
{
    EventBusGuard bus;
    registerTestCalibrator();

    ImageProduct p;
    addChannel(p, 0, 16, 6, 4, "test_unit");
    p.set_calibration("unit_test_linear", {{"gain", 2.0}});

    float progress = 0;
    auto out = satdump::products::generate_calibrated_product_channel(&p, "ch0", 10, 110, "test_unit", &progress);
    REQUIRE(out.width() == 6);
    REQUIRE(out.height() == 4);
    REQUIRE(out.depth() == 16);
    CHECK(progress > 0.7f);

    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 6; x++)
        {
            const double v = 2.0 * (10 * x + y);
            const double expected = std::min(1.0, std::max(0.0, (v - 10) / 100));
            CHECK_THAT(out.getf(0, x, y), WithinAbs(expected, 1.0 / 65535));
        }
    }

    // The output knows how to turn pixel values back into calibrated values
    REQUIRE(satdump::image::has_metadata_calib_cfg(out));
    auto calib = satdump::image::get_metadata_calib_cfg(out);
    CHECK(calib["unit"] == "test_unit");
    CHECK(calib["offset"] == 10.0);
    CHECK(calib["scale"] == 100.0);
}

TEST_CASE("generate_calibrated_product_channel converts units on the way", "[products][image_calibrator]")
{
    EventBusGuard bus;
    registerTestCalibrator();

    ImageProduct p;
    addChannel(p, 0, 16, 4, 4, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE);
    p.set_calibration("unit_test_linear", {{"gain", 10.0}}); // 0..330 K

    auto out = satdump::products::generate_calibrated_product_channel(&p, "ch0", -100, 100, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE_CELSIUS);
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            const double c = 10.0 * (10 * x + y) - 273.15;
            const double expected = std::min(1.0, std::max(0.0, (c + 100) / 200));
            CHECK_THAT(out.getf(0, x, y), WithinAbs(expected, 1.0 / 65535));
        }
    }
    CHECK(satdump::image::get_metadata_calib_cfg(out)["unit"] == CALIBRATION_ID_BRIGHTNESS_TEMPERATURE_CELSIUS);

    // Impossible conversions give the invalid value, which clamps to 0
    auto bad = satdump::products::generate_calibrated_product_channel(&p, "ch0", 0, 1, CALIBRATION_ID_SUN_ANGLE_COMPENSATED_ALBEDO);
    for (size_t i = 0; i < bad.size(); i++)
        CHECK(bad.get(i) == 0);
}

TEST_CASE("generate_calibrated_product_channel attaches the channel projection", "[products][image_calibrator]")
{
    EventBusGuard bus;
    registerTestCalibrator();

    ImageProduct p;
    addChannel(p, 2, 8, 4, 2, "u");
    p.set_calibration("unit_test_linear", {{"gain", 1.0}});
    p.set_proj_cfg({{"type", "equirec"}, {"offset_x", -180.0}, {"offset_y", 90.0}, {"scalar_x", 90.0}, {"scalar_y", -90.0}});

    auto out = satdump::products::generate_calibrated_product_channel(&p, "ch2", 0, 100, "u");
    REQUIRE(satdump::image::has_metadata_proj_cfg(out));
    auto proj = satdump::image::get_metadata_proj_cfg(out);
    CHECK(proj["type"] == "equirec");
    CHECK(proj["width"] == 4);
    CHECK(proj["height"] == 2);
    CHECK(proj.contains("transform"));

    CHECK_THROWS(satdump::products::generate_calibrated_product_channel(&p, "missing", 0, 100, "u"));
}
