#include "image/meta.h"
#include "products/image/image_calibrator.h"
#include "products/image/product_expression.h"
#include "products/products_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdlib>

using Catch::Matchers::WithinAbs;
using products_test::EventBusGuard;
using satdump::products::check_expression_product_composite;
using satdump::products::generate_expression_product_composite;
using satdump::products::ImageProduct;

namespace
{
    const int W = 6, H = 4;

    // ch1: 1000 * (x + 1) + 100 * y, ch2: 20000 + 10 * x, both 16-bit at full resolution.
    // ch3: half resolution (3x2), 40000 + 1000 * x + 100 * y, with a 2x affine transform to the reference grid.
    int v1(int x, int y) { return 1000 * (x + 1) + 100 * y; }
    int v2(int x, int) { return 20000 + 10 * x; }
    int v3(int x, int y) { return 40000 + 1000 * x + 100 * y; }

    void addChannel(ImageProduct &p, int abs_index, int w, int h, int (*f)(int, int))
    {
        ImageProduct::ImageHolder hd;
        hd.abs_index = abs_index;
        hd.channel_name = std::to_string(abs_index);
        hd.image = satdump::image::Image(16, w, h, 1);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                hd.image.set(0, x, y, f(x, y));
        p.images.push_back(hd);
    }

    void buildProduct(ImageProduct &p)
    {
        addChannel(p, 1, W, H, v1);
        addChannel(p, 2, W, H, v2);
        addChannel(p, 3, W / 2, H / 2, v3);
        p.images[2].ch_transform.init_affine(2, 2, 0, 0);
        p.set_channel_wavenumber(2, 1e4 / 10.8);
    }

    // Output values are written as floats in [0, 1] and truncated back to 16 bits
    bool near(int a, double b) { return std::abs(a - b) <= 1.5; }

    class GainCalibrator : public satdump::products::ImageCalibrator
    {
    public:
        GainCalibrator(ImageProduct *p, nlohmann::json c) : ImageCalibrator(p, c) {}

    protected:
        double compute(int, int, int, uint32_t val) override { return val / 1000.0; }
    };
} // namespace

TEST_CASE("generate_expression_product_composite evaluates a single channel", "[products][product_expression]")
{
    ImageProduct p;
    buildProduct(p);
    auto out = generate_expression_product_composite(&p, "ch1");
    REQUIRE(out.width() == W);
    REQUIRE(out.height() == H);
    REQUIRE(out.channels() == 1);
    REQUIRE(out.depth() == 16);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            CHECK(near(out.get(0, x, y), v1(x, y)));
}

TEST_CASE("generate_expression_product_composite produces one output channel per expression", "[products][product_expression]")
{
    ImageProduct p;
    buildProduct(p);
    auto out = generate_expression_product_composite(&p, "ch2, ch1, (ch1 + ch2) / 2");
    REQUIRE(out.channels() == 3);
    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            CHECK(near(out.get(0, x, y), v2(x, y)));
            CHECK(near(out.get(1, x, y), v1(x, y)));
            CHECK(near(out.get(2, x, y), (v1(x, y) + v2(x, y)) / 2.0));
        }
    }
}

TEST_CASE("generate_expression_product_composite clamps results to [0, 1]", "[products][product_expression]")
{
    ImageProduct p;
    buildProduct(p);
    auto out = generate_expression_product_composite(&p, "ch1 * 100, ch1 - 2");
    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            CHECK(out.get(0, x, y) == 65535);
            CHECK(out.get(1, x, y) == 0);
        }
    }
}

TEST_CASE("generate_expression_product_composite ignores whitespace and newlines", "[products][product_expression]")
{
    ImageProduct p;
    buildProduct(p);
    auto out = generate_expression_product_composite(&p, "  ch1 *\r\n 0.5 ,\n ch2 ");
    REQUIRE(out.channels() == 2);
    CHECK(near(out.get(0, 2, 1), v1(2, 1) * 0.5));
    CHECK(near(out.get(1, 2, 1), v2(2, 1)));
}

TEST_CASE("generate_expression_product_composite expands macros", "[products][product_expression]")
{
    ImageProduct p;
    buildProduct(p);
    auto out = generate_expression_product_composite(&p, "half=ch1*0.5; other=ch2; half, other");
    REQUIRE(out.channels() == 2);
    CHECK(near(out.get(0, 3, 2), v1(3, 2) * 0.5));
    CHECK(near(out.get(1, 3, 2), v2(3, 2)));
}

TEST_CASE("generate_expression_product_composite resolves {wavelength} channel tokens", "[products][product_expression]")
{
    ImageProduct p;
    buildProduct(p);
    auto out = generate_expression_product_composite(&p, "ch{10.8um}");
    CHECK(near(out.get(0, 4, 3), v2(4, 3)));
    CHECK_THROWS(generate_expression_product_composite(&p, "ch{3.7um}"));
}

TEST_CASE("generate_expression_product_composite samples other channels through their transforms", "[products][product_expression]")
{
    ImageProduct p;
    buildProduct(p);

    // Reference is the first channel (ch1, full resolution): ch3 is sampled at (x / 2, y / 2)
    auto out = generate_expression_product_composite(&p, "ch1 * 0 + ch3");
    REQUIRE(out.width() == W);
    REQUIRE(out.height() == H);
    for (int y = 0; y < H; y += 2)
        for (int x = 0; x < W; x += 2)
            CHECK(near(out.get(0, x, y), v3(x / 2, y / 2)));
    CHECK(near(out.get(0, 1, 0), (v3(0, 0) + v3(1, 0)) / 2.0)); // Bilinear in between

    // With ch3 as the reference the output is at its resolution and ch1 is sampled at (2x, 2y)
    auto out2 = generate_expression_product_composite(&p, "ref_channel=3; ch1 + ch3 * 0");
    REQUIRE(out2.width() == W / 2);
    REQUIRE(out2.height() == H / 2);
    for (int y = 0; y < H / 2; y++)
        for (int x = 0; x < W / 2; x++)
            CHECK(near(out2.get(0, x, y), v1(2 * x, 2 * y)));

    CHECK_THROWS(generate_expression_product_composite(&p, "ref_channel=2; ch1")); // Reference must be used
}

TEST_CASE("generate_expression_product_composite uses calibrated channel definitions", "[products][product_expression]")
{
    EventBusGuard bus;
    satdump::eventBus->register_handler<satdump::products::RequestImageCalibratorEvent>(
        [](const satdump::products::RequestImageCalibratorEvent &evt)
        {
            if (evt.id == "unit_test_gain")
                evt.calibrators.push_back(std::make_shared<GainCalibrator>(evt.products, evt.calib));
        });

    ImageProduct p;
    buildProduct(p);
    p.set_calibration("unit_test_gain", nlohmann::json::object());
    p.set_channel_unit(1, "kilocounts");

    // c1 = raw / 1000 mapped from [0, 10] into [0, 1]
    auto out = generate_expression_product_composite(&p, "c1=(1,kilocounts,0,10); c1");
    REQUIRE(out.channels() == 1);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            CHECK_THAT(out.getf(0, x, y), WithinAbs(std::min(1.0, v1(x, y) / 1000.0 / 10.0), 1e-4));
}

TEST_CASE("generate_expression_product_composite attaches the reference channel projection", "[products][product_expression]")
{
    ImageProduct p;
    buildProduct(p);
    p.set_proj_cfg({{"type", "equirec"}, {"offset_x", -180.0}});
    auto out = generate_expression_product_composite(&p, "ch3");
    REQUIRE(satdump::image::has_metadata_proj_cfg(out));
    auto cfg = satdump::image::get_metadata_proj_cfg(out);
    CHECK(cfg["width"] == W / 2);
    CHECK(cfg["height"] == H / 2);
    CHECK(cfg["transform"] == nlohmann::json(p.images[2].ch_transform));
}

TEST_CASE("generate_expression_product_composite rejects invalid expressions", "[products][product_expression]")
{
    ImageProduct p;
    buildProduct(p);
    CHECK_THROWS(generate_expression_product_composite(&p, "ch9"));                 // Missing channel
    CHECK_THROWS(generate_expression_product_composite(&p, "foo + ch1"));           // Unknown token
    CHECK_THROWS(generate_expression_product_composite(&p, "ch1 +"));               // Syntax error
    CHECK_THROWS(generate_expression_product_composite(&p, "0.5"));                 // No channel at all
    CHECK_THROWS(generate_expression_product_composite(&p, "ch1,ch1,ch1,ch1,ch1")); // Over 4 outputs
}

TEST_CASE("generate_expression_product_composite dummy mode only checks the expression", "[products][product_expression]")
{
    ImageProduct p;
    buildProduct(p);
    float progress = -1;
    auto out = generate_expression_product_composite(&p, "ch1, ch2", &progress);
    CHECK(out.size() == 0);
    CHECK(progress == 1);
}

TEST_CASE("check_expression_product_composite reports whether a composite can be made", "[products][product_expression]")
{
    ImageProduct p;
    buildProduct(p);
    CHECK(check_expression_product_composite(&p, "ch1, ch2, ch3"));
    CHECK_FALSE(check_expression_product_composite(&p, "ch1, ch7"));
    CHECK_FALSE(check_expression_product_composite(&p, "ch1 +"));
    // Calibrated channels need product calibration, except for the special equalized/sun_angle units
    CHECK_FALSE(check_expression_product_composite(&p, "c1=(1,albedo,0,1); c1"));
    CHECK(check_expression_product_composite(&p, "c1=(1,equalized,0,1); c1"));
    CHECK_FALSE(check_expression_product_composite(&p, "c1=(5,equalized,0,1); c1"));
}
