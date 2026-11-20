#include "common/calibration.h"
#include "products/image_product.h"
#include "products_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using satdump::products::ImageProduct;
using satdump_test_products::ConfigGuard;
using satdump_test_products::TempDir;

namespace
{
    satdump::image::Image patternImage(int depth, int w, int h, int seed)
    {
        satdump::image::Image img(depth, w, h, 1);
        const int maxv = (1 << depth) - 1;
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                img.set(0, x, y, (seed * 7919 + x * 131 + y * 977) % (maxv + 1));
        return img;
    }

    ImageProduct::ImageHolder holder(int abs_index, std::string name, satdump::image::Image img)
    {
        ImageProduct::ImageHolder h;
        h.abs_index = abs_index;
        h.filename = "channel_" + name;
        h.channel_name = name;
        h.image = img;
        return h;
    }

    // Wavenumber (cm^-1) of a wavelength in micrometers
    double wnFromUm(double um) { return 1e4 / um; }

    void requireSameImage(satdump::image::Image &a, satdump::image::Image &b)
    {
        REQUIRE(a.width() == b.width());
        REQUIRE(a.height() == b.height());
        REQUIRE(a.channels() == b.channels());
        REQUIRE(a.depth() == b.depth());
        for (size_t i = 0; i < a.size(); i++)
            if (a.get(i) != b.get(i))
                FAIL("pixel " << i << " differs: " << a.get(i) << " vs " << b.get(i));
    }
} // namespace

TEST_CASE("ImageProduct defaults", "[products][image_product]")
{
    ImageProduct p;
    CHECK(p.type == "image");
    CHECK(p.images.empty());
    CHECK_FALSE(p.has_proj_cfg());
    CHECK_FALSE(p.has_calibration());
    CHECK(p.get_calibration_raw().is_null());

    ImageProduct::ImageHolder h;
    CHECK(h.abs_index == -1);
    CHECK(h.bit_depth == 16);
    CHECK(h.wavenumber == -1);
    CHECK(h.bandwidth == -1);
    CHECK(h.polarization == ImageProduct::POL_NONE);
    CHECK(h.calibration_type.empty());
    CHECK(h.ch_transform.getType() == satdump::ChannelTransform::TYPE_NONE);
}

TEST_CASE("ImageProduct channel lookup by index and name", "[products][image_product]")
{
    ImageProduct p;
    p.images.push_back(holder(3, "3", patternImage(8, 2, 2, 1)));
    p.images.push_back(holder(7, "4a", patternImage(8, 2, 2, 2)));

    CHECK(&p.get_channel_image(3) == &p.images[0]);
    CHECK(&p.get_channel_image(7) == &p.images[1]);
    CHECK(&p.get_channel_image("4a") == &p.images[1]);
    CHECK(p.get_channel_image_idx("4a") == 1);
    CHECK(p.get_channel_image_idx("3") == 0);

    CHECK_THROWS(p.get_channel_image(0));
    CHECK_THROWS(p.get_channel_image("7"));
    CHECK_THROWS(p.get_channel_image_idx("missing"));
}

TEST_CASE("ImageProduct channel metadata setters and getters", "[products][image_product]")
{
    ImageProduct p;
    p.images.push_back(holder(1, "1", patternImage(8, 2, 2, 1)));

    p.set_channel_wavenumber(1, 925.5);
    CHECK(p.get_channel_wavenumber(1) == 925.5);

    // 89 GHz: wavenumber = f / c in cm^-1
    p.set_channel_frequency(1, 89e9);
    CHECK_THAT(p.get_channel_wavenumber(1), WithinRel(89e9 / 299792458.0 / 100.0, 1e-12));
    CHECK_THAT(p.get_channel_frequency(1), WithinRel(89e9, 1e-12));

    p.set_channel_polarization(1, ImageProduct::POL_VERTICAL);
    CHECK(p.get_channel_polarization(1) == ImageProduct::POL_VERTICAL);
    p.set_channel_bandwidth(1, 4e9);
    CHECK(p.get_channel_bandwidth(1) == 4e9);
    p.set_channel_unit(1, "brightness_temperature");
    CHECK(p.images[0].calibration_type == "brightness_temperature");

    CHECK_THROWS(p.set_channel_wavenumber(2, 1));
    CHECK_THROWS(p.set_channel_frequency(2, 1));
    CHECK_THROWS(p.get_channel_wavenumber(2));
    CHECK_THROWS(p.set_channel_polarization(2, ImageProduct::POL_HORIZONTAL));
    CHECK_THROWS(p.get_channel_polarization(2));
    CHECK_THROWS(p.set_channel_bandwidth(2, 1));
    CHECK_THROWS(p.get_channel_bandwidth(2));
    CHECK_THROWS(p.set_channel_unit(2, "x"));
}

TEST_CASE("ImageProduct get_raw_channel_val scales to the channel bit depth", "[products][image_product]")
{
    ImageProduct p;
    auto img8 = satdump::image::Image(8, 2, 1, 1);
    img8.set(0, 0, 0, 200);
    img8.set(0, 1, 0, 3);
    p.images.push_back(holder(0, "a", img8));
    p.images[0].bit_depth = 10; // 8-bit image of a 10-bit instrument: shift up by 2

    auto img16 = satdump::image::Image(16, 1, 1, 1);
    img16.set(0, 0, 0, 0xABCD);
    p.images.push_back(holder(1, "b", img16));
    p.images[1].bit_depth = 12; // 16-bit image of a 12-bit instrument: shift down by 4

    auto img16b = img16;
    p.images.push_back(holder(2, "c", img16b)); // 16 bit, 16 bit: unchanged

    CHECK(p.get_raw_channel_val(0, 0, 0) == 800);
    CHECK(p.get_raw_channel_val(0, 1, 0) == 12);
    CHECK(p.get_raw_channel_val(1, 0, 0) == 0xABC);
    CHECK(p.get_raw_channel_val(2, 0, 0) == 0xABCD);
}

TEST_CASE("ImageProduct get_channel_image_by_specifications picks the closest channel", "[products][image_product]")
{
    ImageProduct p;
    const double um[] = {0.63, 3.7, 10.8, 12.0};
    for (int i = 0; i < 4; i++)
    {
        p.images.push_back(holder(i, std::to_string(i + 1), patternImage(8, 1, 1, i)));
        p.set_channel_wavenumber(i, wnFromUm(um[i]));
    }
    p.images.push_back(holder(9, "nowl", patternImage(8, 1, 1, 9))); // No wavenumber: never matched

    CHECK(p.get_channel_image_by_specifications(wnFromUm(10.8)).channel_name == "3");
    CHECK(p.get_channel_image_by_specifications(wnFromUm(11.0)).channel_name == "3");  // 1.8% off
    CHECK(p.get_channel_image_by_specifications(wnFromUm(11.6)).channel_name == "4");  // Closer to 12.0
    CHECK(p.get_channel_image_by_specifications(wnFromUm(0.65)).channel_name == "1");
    CHECK_THROWS(p.get_channel_image_by_specifications(wnFromUm(6.7)));               // Nothing within 5%
    CHECK(p.get_channel_image_by_specifications(wnFromUm(6.7), ImageProduct::POL_ANY, -1, 50).channel_name == "3"); // Wider tolerance
}

TEST_CASE("ImageProduct get_channel_image_by_specifications resolves sub-unit wavenumber differences", "[products][image_product]")
{
    ImageProduct p;
    p.images.push_back(holder(0, "a", patternImage(8, 1, 1, 0)));
    p.images.push_back(holder(1, "b", patternImage(8, 1, 1, 1)));
    p.set_channel_wavenumber(0, 1000.2);
    p.set_channel_wavenumber(1, 1000.7);
    CHECK(p.get_channel_image_by_specifications(1000.6).channel_name == "b");
}

TEST_CASE("ImageProduct get_channel_image_by_specifications filters polarization and bandwidth", "[products][image_product]")
{
    ImageProduct p;
    const double f = 89e9;
    p.images.push_back(holder(0, "89H", patternImage(8, 1, 1, 0)));
    p.images.push_back(holder(1, "89V", patternImage(8, 1, 1, 1)));
    p.images.push_back(holder(2, "89V_narrow", patternImage(8, 1, 1, 2)));
    for (int i = 0; i < 3; i++)
        p.set_channel_frequency(i, f);
    p.set_channel_polarization(0, ImageProduct::POL_HORIZONTAL);
    p.set_channel_polarization(1, ImageProduct::POL_VERTICAL);
    p.set_channel_polarization(2, ImageProduct::POL_VERTICAL);
    p.set_channel_bandwidth(0, 4e9);
    p.set_channel_bandwidth(1, 4e9);
    p.set_channel_bandwidth(2, 1e9);

    const double wn = freq_to_wavenumber(f);
    CHECK(p.get_channel_image_by_specifications(wn, ImageProduct::POL_HORIZONTAL).channel_name == "89H");
    CHECK(p.get_channel_image_by_specifications(wn, ImageProduct::POL_VERTICAL, 4e9).channel_name == "89V");
    CHECK(p.get_channel_image_by_specifications(wn, ImageProduct::POL_VERTICAL, 1.1e9).channel_name == "89V_narrow");
    CHECK_THROWS(p.get_channel_image_by_specifications(wn, ImageProduct::POL_RHCP));
    CHECK_THROWS(p.get_channel_image_by_specifications(wn, ImageProduct::POL_VERTICAL, 2.5e9)); // No bandwidth within 20%
}

TEST_CASE("ImageProduct get_channel_image_by_unitstr parses wavelength, frequency, polarization and bandwidth", "[products][image_product]")
{
    ImageProduct p;
    p.images.push_back(holder(0, "vis", patternImage(8, 1, 1, 0)));
    p.images.push_back(holder(1, "ir", patternImage(8, 1, 1, 1)));
    p.images.push_back(holder(2, "mwH", patternImage(8, 1, 1, 2)));
    p.images.push_back(holder(3, "mwV", patternImage(8, 1, 1, 3)));
    p.set_channel_wavenumber(0, wnFromUm(0.63));
    p.set_channel_wavenumber(1, wnFromUm(10.8));
    p.set_channel_frequency(2, 89e9);
    p.set_channel_frequency(3, 89e9);
    p.set_channel_polarization(2, ImageProduct::POL_HORIZONTAL);
    p.set_channel_polarization(3, ImageProduct::POL_VERTICAL);
    p.set_channel_bandwidth(2, 2e9);
    p.set_channel_bandwidth(3, 6e9);

    CHECK(p.get_channel_image_by_unitstr("10.8um").channel_name == "ir");
    CHECK(p.get_channel_image_by_unitstr("630nm").channel_name == "vis");
    CHECK(p.get_channel_image_by_unitstr("11um").channel_name == "ir");
    CHECK(p.get_channel_image_by_unitstr("89GHz,H").channel_name == "mwH");
    CHECK(p.get_channel_image_by_unitstr("89GHz,V").channel_name == "mwV");
    CHECK(p.get_channel_image_by_unitstr("89.5GHz,V,6GHz").channel_name == "mwV");
    CHECK(p.get_channel_image_by_unitstr("89GHz,*,2GHz").channel_name == "mwH");
    CHECK(p.get_channel_image_by_unitstr("89GHz,*,-1").channel_name != "");
    CHECK(p.get_channel_image_by_unitstr("95GHz,V,-1,10").channel_name == "mwV"); // 6.7% off, 10% tolerance
    CHECK_THROWS(p.get_channel_image_by_unitstr("95GHz,V"));                      // 5% default tolerance
    CHECK(p.get_channel_image_by_unitstr("89GHz,V,4GHz,5,60").channel_name == "mwV"); // 50% bandwidth tolerance needed

    CHECK_THROWS(p.get_channel_image_by_unitstr("10.8"));          // No unit
    CHECK_THROWS(p.get_channel_image_by_unitstr("89GHz,X"));       // Bad polarization
    CHECK_THROWS(p.get_channel_image_by_unitstr("89GHz,V,banana")); // Bad bandwidth
    CHECK_THROWS(p.get_channel_image_by_unitstr("89GHz,L"));       // No such channel
}

TEST_CASE("ImageProduct projection config helpers", "[products][image_product]")
{
    ImageProduct p;
    p.images.push_back(holder(2, "2", patternImage(8, 40, 30, 1)));
    p.images[0].ch_transform.init_affine(2, 2, 0, 0);

    nlohmann::json tle = {{"norad", 1}, {"name", "TESTSAT"}, {"line1", "l1"}, {"line2", "l2"}};
    p.set_proj_cfg_tle_timestamps({{"type", "normal_single_line"}}, tle, {10.0, 30.0, 20.0, -1.0, 40.0});
    REQUIRE(p.has_proj_cfg());
    CHECK(p.contents["projection_cfg"]["type"] == "normal_single_line");
    CHECK(p.contents["projection_cfg"]["tle"] == tle);
    // The source comes from the TLE name and the timestamp is the median of the timestamps
    CHECK(p.get_product_source() == "TESTSAT");
    CHECK(p.get_product_timestamp() == 20.0);

    // Existing source/timestamp are not overwritten
    p.set_product_source("Kept");
    p.set_product_timestamp(5);
    p.set_proj_cfg_tle_timestamps({{"type", "x"}}, tle, {100.0});
    CHECK(p.get_product_source() == "Kept");
    CHECK(p.get_product_timestamp() == 5);

    // Channel-specific config adds transform, size and the product timestamp
    auto cfg = p.get_proj_cfg(2);
    CHECK(cfg["type"] == "x");
    CHECK(cfg["width"] == 40);
    CHECK(cfg["height"] == 30);
    CHECK(cfg["proj_timestamp"] == 5.0);
    CHECK(cfg["transform"] == nlohmann::json(p.images[0].ch_transform));

    auto raw = p.get_proj_cfg(-1);
    CHECK_FALSE(raw.contains("width"));
    CHECK_FALSE(raw.contains("transform"));
    CHECK_THROWS(p.get_proj_cfg(3));
}

// Bug: image_product.h:125 reads contents["projection_cfg"] through the non-const
// operator[], which inserts a null "projection_cfg" when there is none. Asking a product
// without projection for a channel config makes has_proj_cfg() true from then on.
TEST_CASE("ImageProduct get_proj_cfg does not add a projection to the product", "[products][image_product][!shouldfail]")
{
    ImageProduct p;
    p.images.push_back(holder(0, "0", patternImage(8, 4, 4, 1)));
    REQUIRE_FALSE(p.has_proj_cfg());
    p.get_proj_cfg(0);
    CHECK_FALSE(p.has_proj_cfg());
}

TEST_CASE("ImageProduct calibration config helpers", "[products][image_product]")
{
    ImageProduct p;
    p.set_calibration("my_calibrator", {{"coefs", {1, 2, 3}}});
    REQUIRE(p.has_calibration());
    auto c = p.get_calibration();
    CHECK(c.first == "my_calibrator");
    CHECK(c.second["coefs"] == nlohmann::json{1, 2, 3});
    CHECK(c.second["calibrator"] == "my_calibrator");
    CHECK(p.get_calibration_raw() == c.second);
}

TEST_CASE("ImageProduct save/load round-trips images and channel metadata", "[products][image_product]")
{
    ConfigGuard cfg;
    cfg.setProductFormat("png");
    TempDir dir("image_product");

    ImageProduct p;
    p.instrument_name = "test_imager";
    p.set_product_timestamp(1700000000);
    p.set_proj_cfg({{"type", "equirec"}, {"offset_x", -180.0}});
    p.set_calibration("cal", {{"k", 1.5}});
    p.images.push_back(holder(0, "1", patternImage(16, 8, 4, 1)));
    p.images.push_back(holder(5, "IR", patternImage(8, 5, 3, 2)));
    p.images[0].bit_depth = 12;
    p.images[0].ch_transform.init_affine(2, 1, 0.5, 0);
    p.images[0].wavenumber = 925.5;
    p.images[0].polarization = ImageProduct::POL_LHCP;
    p.images[0].bandwidth = 12.5;
    p.images[0].calibration_type = "emissive_radiance";
    p.images[1].bit_depth = 8;
    p.save(dir.str());

    CHECK(std::filesystem::exists(dir.path / "product.cbor"));
    CHECK(std::filesystem::exists(dir.path / "channel_1.png"));
    CHECK(std::filesystem::exists(dir.path / "channel_IR.png"));
    CHECK(p.images[0].filename == "channel_1.png"); // The extension is appended in place

    ImageProduct q;
    q.load((dir.path / "product.cbor").string());
    CHECK(q.instrument_name == "test_imager");
    CHECK(q.get_product_timestamp() == 1700000000);
    CHECK(q.contents["projection_cfg"] == p.contents["projection_cfg"]);
    CHECK(q.get_calibration().first == "cal");
    REQUIRE(q.images.size() == 2);

    for (int i = 0; i < 2; i++)
    {
        auto &a = p.images[i];
        auto &b = q.images[i];
        CHECK(b.abs_index == a.abs_index);
        CHECK(b.filename == a.filename);
        CHECK(b.channel_name == a.channel_name);
        CHECK(b.bit_depth == a.bit_depth);
        CHECK(nlohmann::json(b.ch_transform) == nlohmann::json(a.ch_transform));
        CHECK(b.wavenumber == a.wavenumber);
        CHECK(b.polarization == a.polarization);
        CHECK(b.bandwidth == a.bandwidth);
        CHECK(b.calibration_type == a.calibration_type);
        requireSameImage(b.image, a.image);
    }
    CHECK(q.images[1].wavenumber == -1); // Unset optional fields stay unset
    CHECK(q.images[1].polarization == ImageProduct::POL_NONE);
}

TEST_CASE("ImageProduct save/load as a single matrix image", "[products][image_product]")
{
    ConfigGuard cfg;
    cfg.setProductFormat("png");
    TempDir dir("image_matrix");

    ImageProduct p;
    p.instrument_name = "matrix";
    p.save_as_matrix = true;
    for (int i = 0; i < 3; i++)
        p.images.push_back(holder(i, std::to_string(i), patternImage(16, 6, 4, i + 1)));
    p.save(dir.str());

    // Only the first file name is written, holding a 2x2 grid of channels
    CHECK(std::filesystem::exists(dir.path / "channel_0.png"));
    CHECK_FALSE(std::filesystem::exists(dir.path / "channel_1.png"));
    CHECK(p.contents["img_matrix_size"] == 2);

    ImageProduct q;
    q.load((dir.path / "product.cbor").string());
    CHECK(q.save_as_matrix);
    REQUIRE(q.images.size() == 3);
    for (int i = 0; i < 3; i++)
        requireSameImage(q.images[i].image, p.images[i].image);
}

TEST_CASE("ImageProduct d_no_not_save_images / d_no_not_load_images skip image files", "[products][image_product]")
{
    ConfigGuard cfg;
    cfg.setProductFormat("png");
    TempDir dir("image_noimg");

    ImageProduct p;
    p.instrument_name = "meta_only";
    p.d_no_not_save_images = true;
    p.images.push_back(holder(0, "0", patternImage(8, 4, 4, 1)));
    p.save(dir.str());
    CHECK(std::filesystem::exists(dir.path / "product.cbor"));
    CHECK_FALSE(std::filesystem::exists(dir.path / "channel_0.png"));

    ImageProduct q;
    q.load((dir.path / "product.cbor").string());
    REQUIRE(q.images.size() == 1);
    CHECK(q.images[0].channel_name == "0");
    CHECK(q.images[0].image.size() == 0); // The file is missing, so the image stays empty

    // With the image present, d_no_not_load_images still skips it
    ImageProduct p2;
    p2.instrument_name = "with_img";
    p2.images.push_back(holder(0, "0", patternImage(8, 4, 4, 1)));
    p2.save(dir.str());
    ImageProduct q2;
    q2.d_no_not_load_images = true;
    q2.load((dir.path / "product.cbor").string());
    REQUIRE(q2.images.size() == 1);
    CHECK(q2.images[0].image.size() == 0);
}

TEST_CASE("ImageProduct save falls back to PNG without a configured format", "[products][image_product]")
{
    ConfigGuard cfg; // Restores the null keys getValueFromSatDumpGeneral() inserts
    satdump::satdump_cfg.main_cfg = nlohmann::ordered_json::object();
    TempDir dir("image_default");

    ImageProduct p;
    p.instrument_name = "default_fmt";
    p.images.push_back(holder(0, "0", patternImage(8, 3, 3, 1)));
    p.save(dir.str());
    CHECK(std::filesystem::exists(dir.path / "channel_0.png"));
}
