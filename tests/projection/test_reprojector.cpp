#include "image/meta.h"
#include "projection/reproject_img.h"
#include "projection/reprojector.h"
#include "projection/warp/warp_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdlib>

using satdump::projection::ReprojectionOperation;
using warp_test::gradientImage;

namespace
{
    // Full-globe equirectangular config with `deg` degrees per pixel
    nlohmann::json globeCfg(double deg)
    {
        return {{"type", "equirec"}, {"offset_x", -180.0}, {"offset_y", 90.0}, {"scalar_x", deg}, {"scalar_y", -deg}, {"width", (int)(360 / deg)}, {"height", (int)(180 / deg)}};
    }

    // 36x18 global image (10 degree pixels) with gradientImage() values
    satdump::image::Image globeImage(int channels = 1)
    {
        auto img = gradientImage(36, 18, channels);
        satdump::image::set_metadata_proj_cfg(img, globeCfg(10));
        return img;
    }
} // namespace

TEST_CASE("reproject rejects empty images and images without projection", "[projection][reprojector]")
{
    satdump::image::Image empty;
    ReprojectionOperation op;
    op.img = &empty;
    op.output_width = 10;
    op.output_height = 10;
    op.target_prj_info = globeCfg(36);
    CHECK_THROWS(satdump::projection::reproject(op));

    satdump::image::Image img(16, 4, 4, 1);
    op.img = &img;
    CHECK_THROWS(satdump::projection::reproject(op));
}

TEST_CASE("reproject between two equirectangular grids resamples the image", "[projection][reprojector]")
{
    auto img = globeImage();
    ReprojectionOperation op;
    op.img = &img;
    op.output_width = 72;
    op.output_height = 36;
    op.target_prj_info = globeCfg(5);
    auto out = satdump::projection::reproject(op);

    REQUIRE(out.width() == 72);
    REQUIRE(out.height() == 36);
    REQUIRE(out.channels() == 4);
    REQUIRE(out.depth() == 16);

    // Target pixel (x, y) is source pixel (x / 2, y / 2)
    size_t mismatches = 0;
    int first_x = 0, first_y = 0;
    for (int y = 0; y < 34; y++)
    {
        for (int x = 0; x < 70; x++)
        {
            const double expected = 1000 * (x / 2.0) + 100 * (y / 2.0);
            bool ok = out.get(3, x, y) == 65535;
            for (int c = 0; c < 3; c++)
                ok &= std::abs(out.get(c, x, y) - expected) <= 1;
            if (!ok && mismatches++ == 0)
                first_x = x, first_y = y;
        }
    }
    INFO("first mismatch at x=" << first_x << " y=" << first_y);
    CHECK(mismatches == 0);

    // The target projection is attached to the output
    REQUIRE(satdump::image::has_metadata_proj_cfg(out));
    CHECK(satdump::image::get_metadata_proj_cfg(out) == globeCfg(5));
}

TEST_CASE("reproject keeps RGB channels separate", "[projection][reprojector]")
{
    auto img = globeImage(3);
    ReprojectionOperation op;
    op.img = &img;
    op.output_width = 36;
    op.output_height = 18;
    op.target_prj_info = globeCfg(10);
    auto out = satdump::projection::reproject(op);

    REQUIRE(out.channels() == 4);
    for (int y = 0; y < 18; y++)
    {
        for (int x = 0; x < 36; x++)
        {
            for (int c = 0; c < 3; c++)
                CHECK(out.get(c, x, y) == img.get(c, x, y));
            CHECK(out.get(3, x, y) == 65535);
        }
    }
}

TEST_CASE("reproject converts 8-bit input to 16 bits", "[projection][reprojector]")
{
    satdump::image::Image img(8, 36, 18, 1);
    for (int y = 0; y < 18; y++)
        for (int x = 0; x < 36; x++)
            img.set(0, x, y, x + y);
    satdump::image::set_metadata_proj_cfg(img, globeCfg(10));

    ReprojectionOperation op;
    op.img = &img;
    op.output_width = 36;
    op.output_height = 18;
    op.target_prj_info = globeCfg(10);
    auto out = satdump::projection::reproject(op);

    REQUIRE(out.depth() == 16);
    CHECK(out.get(0, 5, 7) == (12 << 8));
    CHECK(out.get(0, 30, 2) == (32 << 8));
}

TEST_CASE("reproject into a geostationary view leaves space empty", "[projection][reprojector]")
{
    auto img = globeImage();
    // 50x50 view of the full disk, 250 km pixels in projection units, centered on 0E
    nlohmann::json geos = {{"type", "geos"}, {"altitude", 35786023.0}, {"scalar_x", 250000.0}, {"scalar_y", -250000.0}, {"offset_x", -6.25e6}, {"offset_y", 6.25e6}, {"width", 50}, {"height", 50}};
    ReprojectionOperation op;
    op.img = &img;
    op.output_width = 50;
    op.output_height = 50;
    op.target_prj_info = geos;
    auto out = satdump::projection::reproject(op);

    // Corners are in space, the center sees 0N 0E = source pixel (18, 9)
    CHECK(out.get(3, 0, 0) == 0);
    CHECK(out.get(3, 49, 49) == 0);
    CHECK(out.get(3, 25, 25) == 65535);
    CHECK(std::abs(out.get(0, 25, 25) - (1000 * 18 + 100 * 9)) <= 1);
}

TEST_CASE("reproject to an invalid target returns an empty canvas", "[projection][reprojector]")
{
    auto img = globeImage();
    ReprojectionOperation op;
    op.img = &img;
    op.output_width = 8;
    op.output_height = 4;
    op.target_prj_info = {{"type", "no_such_projection"}, {"width", 8}, {"height", 4}};
    auto out = satdump::projection::reproject(op);
    REQUIRE(out.width() == 8);
    REQUIRE(out.height() == 4);
    for (size_t i = 0; i < out.size(); i++)
        CHECK(out.get(i) == 0);
}

TEST_CASE("reprojectImage takes size from the target projection", "[projection][reprojector]")
{
    auto img = globeImage();
    auto out = satdump::projection::reprojectImage(img, globeCfg(5));
    REQUIRE(out.width() == 72);
    REQUIRE(out.height() == 36);
    CHECK(std::abs(out.get(0, 20, 10) - (1000 * 10 + 100 * 5)) <= 1);
    // The source image's projection gets its size filled in
    CHECK(satdump::image::get_metadata_proj_cfg(img)["width"] == 36);
}

TEST_CASE("reproject on a source projection without size throws", "[projection][reprojector]")
{
    // Projection::init() requires width/height, and reproject() does not add them for
    // standard sources either; reprojectImage() is the entry point that does
    auto img = globeImage();
    auto cfg = globeCfg(10);
    cfg.erase("width");
    satdump::image::set_metadata_proj_cfg(img, cfg);
    ReprojectionOperation op;
    op.img = &img;
    op.output_width = 36;
    op.output_height = 18;
    op.target_prj_info = globeCfg(10);
    CHECK_THROWS(satdump::projection::reproject(op));
}

// Needs CpuOnlyWarpGuard, which uses a data symbol of satdump_core that is not
// exported from the Windows DLL (see warp_test_helpers.h)
#ifndef _MSC_VER
using warp_test::CpuOnlyWarpGuard;

TEST_CASE("reproject warps a raytraced (GCP) source through the smart warper", "[projection][reprojector][warp]")
{
    CpuOnlyWarpGuard cpu_only;

    // 20x10 swath near the equator: pixel (x, y) at lon = 10 + 0.5x, lat = 2 - 0.5y
    auto img = gradientImage(20, 10, 1);
    nlohmann::json cfg;
    cfg["type"] = "gcps_timestamps_line";
    int n = 0;
    for (int y = 0; y < 10; y++)
        for (int x : {0, 19})
            cfg["gcps"][n++] = {{"x", x}, {"y", y}, {"lat", 2 - 0.5 * y}, {"lon", 10 + 0.5 * x}};
    cfg["gcp_cnt"] = n;
    cfg["gcp_spacing_x"] = 4;
    cfg["gcp_spacing_y"] = 1;
    cfg["f_width"] = 1440; // 0.25 degree intermediate grid
    satdump::image::set_metadata_proj_cfg(img, cfg);

    // reprojectImage() fills in the source width/height the raytracer config needs. Calling
    // reproject() directly on this metadata throws instead (see UNTESTED_ISSUES.md).
    auto out = satdump::projection::reprojectImage(img, globeCfg(0.5));
    REQUIRE(out.width() == 720);
    REQUIRE(out.channels() == 4);

    // Target pixel (x, y) covers lon -180 + 0.5x, lat 90 - 0.5y: input pixel (x - 380, y - 176).
    // The per-line GCP interpolation is not exactly affine off the equator and the image goes
    // through two resamplings, so allow up to a third of a pixel of positional error.
    int covered = 0;
    for (int y = 176; y < 186; y++)
    {
        for (int x = 380; x < 400; x++)
        {
            const int ix = x - 380, iy = y - 176;
            if (ix == 0 || iy == 0 || ix >= 18 || iy >= 8)
                continue; // Edges depend on rounding of the warp crop
            INFO("x=" << x << " y=" << y);
            CHECK(out.get(3, x, y) == 65535);
            CHECK(std::abs(out.get(0, x, y) - (1000 * ix + 100 * iy)) <= 350);
            covered++;
        }
    }
    CHECK(covered == 17 * 7);

    // Far away from the swath nothing is drawn
    CHECK(out.get(3, 10, 10) == 0);
    CHECK(out.get(3, 600, 300) == 0);
}
#endif
