#include "image/earth_curvature.h"
#include "image/meta.h"
#include "image_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cmath>

using satdump::image::Image;
namespace ec = satdump::image::earth_curvature;

TEST_CASE("image::earth_curvature::correct_earth_curvature output size and constant images", "[image][earth_curvature]")
{
    auto depth = GENERATE(8, 16);
    Image in(depth, 100, 3, 2);
    in.fill(depth == 8 ? 77 : 12345);
    // AVHRR-like: 2900 km swath from 830 km, 1.1 km resolution
    Image out = ec::correct_earth_curvature(in, 830, 2900, 1.1f);
    REQUIRE(out.width() == size_t(std::round(2900 / 1.1f)));
    REQUIRE(out.height() == 3);
    REQUIRE(out.channels() == 2);
    REQUIRE(out.depth() == depth);
    for (size_t i = 0; i < out.size(); i++)
        REQUIRE(std::abs(out.get(i) - (depth == 8 ? 77 : 12345)) <= 1);
}

TEST_CASE("image::earth_curvature::correct_earth_curvature lookup tables", "[image][earth_curvature]")
{
    Image in(16, 200, 1, 1);
    std::vector<float> fwd, rev;
    Image out = ec::correct_earth_curvature(in, 830, 2900, 2.0f, &fwd, &rev);
    const int W = 1450;
    REQUIRE(out.width() == size_t(W));
    REQUIRE(rev.size() == size_t(W));
    REQUIRE(fwd.size() == 200);

    // The swath edges map to the first input pixel, nadir to the centre one
    CHECK(rev[0] == 0);
    CHECK(rev[W / 2] == 100);
    CHECK(rev[W - 1] >= 198);

    // Monotonic, and the input is stretched more at the edges than at nadir
    for (int i = 1; i < W; i++)
        REQUIRE(rev[i] >= rev[i - 1]);
    int edge_span = rev[100] - rev[0];
    int centre_span = rev[W / 2 + 50] - rev[W / 2 - 50];
    CHECK(centre_span > edge_span);

    // Every input pixel gets a forward mapping, and it is monotonic too
    for (int i = 1; i < 200; i++)
    {
        REQUIRE(fwd[i] >= fwd[i - 1]);
        REQUIRE(fwd[i] >= 0);
        REQUIRE(fwd[i] < W);
    }
}

TEST_CASE("image::earth_curvature::correct_earth_curvature resamples a ramp monotonically", "[image][earth_curvature]")
{
    Image in(16, 64, 1, 1);
    for (int x = 0; x < 64; x++)
        in.set(0, x, 0, x * 1000);
    Image out = ec::correct_earth_curvature(in, 800, 2000, 10.0f);
    REQUIRE(out.width() == 200);
    CHECK(out.get(0, 0, 0) == 0);
    // Interpolation truncates, so allow one count of jitter
    for (int x = 1; x < 200; x++)
        REQUIRE(out.get(0, x, 0) + 1 >= out.get(0, x - 1, 0));
    CHECK(out.get(0, 199, 0) >= 62000);
}

TEST_CASE("image::earth_curvature::perform_geometric_correction needs metadata", "[image][earth_curvature]")
{
    SECTION("no metadata")
    {
        Image in = image_test::makePattern(8, 20, 2, 1);
        bool success = true;
        Image out = ec::perform_geometric_correction(in, success);
        CHECK_FALSE(success);
        CHECK(image_test::samePixels(out, in));
    }

    SECTION("incomplete correction parameters")
    {
        Image in(8, 20, 2, 1);
        satdump::image::set_metadata_proj_cfg(in, {{"corr_swath", 2000}, {"corr_altit", 800}});
        bool success = true;
        Image out = ec::perform_geometric_correction(in, success);
        CHECK_FALSE(success);
        CHECK(out.width() == 20);
    }

    SECTION("empty image is returned as is")
    {
        Image in;
        bool success = false;
        Image out = ec::perform_geometric_correction(in, success);
        CHECK(out.width() == 0);
    }
}

TEST_CASE("image::earth_curvature::perform_geometric_correction applies and records the correction", "[image][earth_curvature]")
{
    // Single row: the LUTs are written from the per-row parallel loop
    Image in(8, 50, 1, 1);
    in.fill(10);
    satdump::image::set_metadata_proj_cfg(in, {{"corr_swath", 2000.0}, {"corr_resol", 10.0}, {"corr_altit", 800.0}});

    bool success = false;
    std::vector<float> rev;
    Image out = ec::perform_geometric_correction(in, success, nullptr, &rev);
    REQUIRE(success);
    CHECK(out.width() == 200);
    CHECK(out.height() == 1);
    CHECK(rev.size() == 200);
    for (size_t i = 0; i < out.size(); i++)
        REQUIRE(std::abs(out.get(i) - 10) <= 1);

    REQUIRE(satdump::image::has_metadata_proj_cfg(out));
    auto cfg = satdump::image::get_metadata_proj_cfg(out);
    CHECK(cfg.contains("transform2"));
    CHECK(cfg["corr_swath"].get<double>() == 2000.0);
}

TEST_CASE("image::earth_curvature::perform_geometric_correction rescales resolution on width mismatch", "[image][earth_curvature]")
{
    // The parameters are for a 100 pixel wide image, this one is 50 wide,
    // so the effective resolution doubles and the output halves
    Image in(8, 50, 1, 1);
    satdump::image::set_metadata_proj_cfg(in, {{"corr_swath", 2000.0}, {"corr_resol", 10.0}, {"corr_altit", 800.0}, {"corr_width", 100}});
    bool success = false;
    Image out = ec::perform_geometric_correction(in, success);
    REQUIRE(success);
    CHECK(out.width() == 100);
}
