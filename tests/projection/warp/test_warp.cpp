#include "projection/warp/warp.h"
#include "projection/warp/warp_bkd.h"
#include "warp_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <cstdlib>

using Catch::Matchers::WithinAbs;
using satdump::projection::GCP;
using satdump_test_proj_warp::affineGcps;
using satdump_test_proj_warp::CpuOnlyWarpGuard;
using satdump_test_proj_warp::gradientImage;

namespace satdump
{
    namespace warp
    {
        // Internal helpers from warp.cpp / smart_warp.cpp (not in any header, also used by latlontps_proj.cpp)
        double lon_shift(double lon, double shift);
        void shift_latlon_by_lat(double *lat, double *lon, double shift);
        void computeGCPCenter(std::vector<satdump::projection::GCP> &gcps, double &lon, double &lat);
    } // namespace warp
} // namespace satdump

TEST_CASE("warp lon_shift shifts and wraps longitudes", "[projection][warp]")
{
    using satdump::warp::lon_shift;
    CHECK(lon_shift(10, 0) == 10);
    CHECK(lon_shift(200, 0) == 200); // A zero shift is a no-op, even out of range
    CHECK(lon_shift(10, 20) == 30);
    CHECK(lon_shift(170, 20) == -170);
    CHECK(lon_shift(-170, -20) == 170);
    CHECK(lon_shift(-175, 180) == 5);
}

TEST_CASE("warp shift_latlon_by_lat rotates the globe about the 90E-90W axis", "[projection][warp]")
{
    using satdump::warp::shift_latlon_by_lat;

    // A +90 rotation brings the north pole to (0, 0) and (0, 0) to the south pole
    double lat = 90, lon = 0;
    shift_latlon_by_lat(&lat, &lon, 90);
    CHECK_THAT(lat, WithinAbs(0, 1e-9));
    CHECK_THAT(lon, WithinAbs(0, 1e-9));

    lat = 0, lon = 0;
    shift_latlon_by_lat(&lat, &lon, 90);
    CHECK_THAT(lat, WithinAbs(-90, 1e-9));

    // Points on the rotation axis do not move
    lat = 0, lon = 90;
    shift_latlon_by_lat(&lat, &lon, 37);
    CHECK_THAT(lat, WithinAbs(0, 1e-9));
    CHECK_THAT(lon, WithinAbs(90, 1e-9));

    // On the prime meridian it is a plain latitude shift
    lat = 10, lon = 0;
    shift_latlon_by_lat(&lat, &lon, -25);
    CHECK_THAT(lat, WithinAbs(35, 1e-9));
    CHECK_THAT(lon, WithinAbs(0, 1e-9));

    // Rotating back restores any point
    for (double la = -80; la <= 80; la += 20)
    {
        for (double lo = -170; lo <= 170; lo += 34)
        {
            double a = la, o = lo;
            shift_latlon_by_lat(&a, &o, 63);
            shift_latlon_by_lat(&a, &o, -63);
            CHECK_THAT(a, WithinAbs(la, 1e-9));
            CHECK_THAT(o, WithinAbs(lo, 1e-9));
        }
    }

    // Zero shift is a no-op
    lat = 12, lon = 34;
    shift_latlon_by_lat(&lat, &lon, 0);
    CHECK(lat == 12);
    CHECK(lon == 34);
}

TEST_CASE("warp computeGCPCenter averages on the sphere", "[projection][warp]")
{
    std::vector<GCP> g = {{0, 0, 170, 10}, {0, 0, -170, 10}, {0, 0, 170, -10}, {0, 0, -170, -10}};
    double lon, lat;
    satdump::warp::computeGCPCenter(g, lon, lat);
    CHECK_THAT(std::fabs(lon), WithinAbs(180, 1e-9)); // Not 0, as a naive average would give
    CHECK_THAT(lat, WithinAbs(0, 1e-9));

    g = {{0, 0, 10, 40}, {0, 0, 20, 40}};
    satdump::warp::computeGCPCenter(g, lon, lat);
    CHECK_THAT(lon, WithinAbs(15, 1e-9));
    CHECK(lat > 40); // The great-circle midpoint bulges polewards
    CHECK(lat < 40.5);
}

namespace
{
    // The synthetic input: 20x10 pixels of 0.5 degrees, with pixel (x, y) at
    // lon = LON0 + 0.5x, lat = LAT0 - 0.5y. LON0/LAT0 put the input a quarter pixel off the
    // 0.5 degree output grid, so the warp always samples at x.25 / y.25 positions and never
    // exactly on a pixel boundary where TPS round-off could flip the result.
    const double LON0 = 29.875, LAT0 = 20.125, RES = 0.5;
    const int IW = 20, IH = 10;

    // Checks every output pixel against the analytic bilinear sample of gradientImage()
    void checkWarpedGradient(satdump::warp::WarpResult &res, int in_channels, bool rgba)
    {
        auto &out = res.output_image;
        const int out_channels = rgba ? 4 : in_channels;
        REQUIRE(out.channels() == out_channels);
        // Output pixels are 0.5 degrees on a 720x360 global grid
        CHECK_THAT(res.top_right.lon - res.top_left.lon, WithinAbs(RES * (out.width() - 0), 1e-9));
        CHECK_THAT(res.top_left.lat - res.bottom_left.lat, WithinAbs(RES * (out.height() - 0), 1e-9));

        int covered = 0;
        for (size_t y = 0; y < out.height(); y++)
        {
            for (size_t x = 0; x < out.width(); x++)
            {
                const double lon = res.top_left.lon + RES * x, lat = res.top_left.lat - RES * y;
                const double xx = (lon - LON0) / RES, yy = (LAT0 - lat) / RES;
                INFO("x=" << x << " y=" << y << " samples input " << xx << ", " << yy);
                // The warp keeps any sample whose integer part is inside the input
                if (xx < 0 || yy < 0 || xx >= IW || yy >= IH)
                {
                    for (int c = 0; c < out_channels; c++)
                        CHECK(out.get(c, x, y) == 0);
                    continue;
                }
                covered++;
                if (rgba)
                    CHECK(out.get(3, x, y) == 65535);
                // Past the last pixel centre the value depends on the image's edge handling
                if (xx > IW - 1 || yy > IH - 1)
                    continue;
                for (int c = 0; c < (rgba ? 3 : in_channels); c++)
                {
                    const int ic = in_channels == 1 ? 0 : c;
                    CHECK(std::abs(out.get(c, x, y) - (1000 * xx + 100 * yy + 7 * ic)) <= 2);
                }
            }
        }
        // One output pixel per input pixel
        CHECK(covered == IW * IH);
    }
} // namespace

TEST_CASE("warp choseCropArea covers the GCPs with whole-degree bounds", "[projection][warp]")
{
    satdump::image::Image img(16, 10, 10, 1);
    satdump::warp::WarpOperation op;
    op.input_image = &img;
    op.output_width = 360;
    op.output_height = 180;
    op.ground_control_points = {{0, 0, 30.1, 20.7}, {9, 0, 40.9, 20.2}, {0, 9, 30.5, 15.2}, {9, 9, 40.2, 15.9}};

    auto c = satdump::warp::choseCropArea(op);
    CHECK(c.lat_min == 15);
    CHECK(c.lat_max == 21);
    CHECK(c.lon_min == 30);
    CHECK(c.lon_max == 41);
    // 1 degree pixels: row 0 is 90N, column 0 is 180W
    CHECK(c.y_min == 69);
    CHECK(c.y_max == 75);
    CHECK(c.x_min == 210);
    CHECK(c.x_max == 221);

    // Polar segments extend to the pole
    op.shift_lat = 90;
    c = satdump::warp::choseCropArea(op);
    CHECK(c.lat_max == 90);
    CHECK(c.y_min == 0);
}

// Bug: warp.cpp:165-168 converts the rounded degree bounds to pixels with float math and
// truncates to int. (90 + 10) / 180.0f * 180 evaluates to 100.00001, so y_max becomes 79
// instead of 80 and the recomputed lat_min is 11: the crop no longer covers the GCPs
// between 10 and 11 degrees and that strip of data is lost from the warp.
TEST_CASE("warp choseCropArea never crops away GCPs", "[projection][warp][!shouldfail]")
{
    satdump::image::Image img(16, 10, 10, 1);
    satdump::warp::WarpOperation op;
    op.input_image = &img;
    op.output_width = 360;
    op.output_height = 180;
    op.ground_control_points = {{0, 0, 30.1, 20.7}, {9, 0, 40.9, 20.2}, {0, 9, 30.5, 10.2}, {9, 9, 40.2, 10.9}};

    auto c = satdump::warp::choseCropArea(op);
    CHECK(c.lat_min <= 10.2);
    CHECK(c.y_max == 80);
}

TEST_CASE("ImageWarper warps an image through an affine GCP grid", "[projection][warp]")
{
    CpuOnlyWarpGuard cpu_only;

    satdump::image::Image img = gradientImage(IW, IH, 1);
    satdump::warp::ImageWarper warper;
    warper.op.input_image = &img;
    warper.op.ground_control_points = affineGcps(IW, IH, LON0, LAT0, RES, 3);
    warper.op.output_width = 720;
    warper.op.output_height = 360;
    warper.update();
    auto res = warper.warp();

    // GCPs span lon 29.875..39.375, lat 15.625..20.125 => whole degrees 29..40, 15..21
    CHECK(res.output_image.width() == 22);
    CHECK(res.output_image.height() == 12);
    CHECK_THAT(res.top_left.lon, WithinAbs(29, 1e-9));
    CHECK_THAT(res.top_left.lat, WithinAbs(21, 1e-9));
    CHECK(res.top_left.x == 0);
    CHECK(res.top_left.y == 0);
    CHECK(res.bottom_right.x == res.output_image.width() - 1);
    CHECK(res.bottom_right.y == res.output_image.height() - 1);
    checkWarpedGradient(res, 1, false);
}

TEST_CASE("ImageWarper warps every channel of a multi-channel image", "[projection][warp]")
{
    CpuOnlyWarpGuard cpu_only;

    satdump::image::Image img = gradientImage(IW, IH, 3);
    satdump::warp::ImageWarper warper;
    warper.op.input_image = &img;
    warper.op.ground_control_points = affineGcps(IW, IH, LON0, LAT0, RES, 3);
    warper.op.output_width = 720;
    warper.op.output_height = 360;
    warper.update();
    auto res = warper.warp();
    checkWarpedGradient(res, 3, false);
}

TEST_CASE("ImageWarper output_rgba expands grayscale and sets alpha on covered pixels", "[projection][warp]")
{
    CpuOnlyWarpGuard cpu_only;

    satdump::image::Image img = gradientImage(IW, IH, 1);
    satdump::warp::ImageWarper warper;
    warper.op.input_image = &img;
    warper.op.ground_control_points = affineGcps(IW, IH, LON0, LAT0, RES, 3);
    warper.op.output_width = 720;
    warper.op.output_height = 360;
    warper.op.output_rgba = true;
    warper.update();
    auto res = warper.warp();
    checkWarpedGradient(res, 1, true);
}

TEST_CASE("performSmartWarp matches the single-segment ImageWarper result", "[projection][warp][smart_warp]")
{
    CpuOnlyWarpGuard cpu_only;

    satdump::image::Image img = gradientImage(IW, IH, 3);
    satdump::warp::WarpOperation op;
    op.input_image = &img;
    op.ground_control_points = affineGcps(IW, IH, LON0, LAT0, RES, 3);
    op.output_width = 720;
    op.output_height = 360;
    op.output_rgba = true;
    float progress = 0;
    auto res = satdump::warp::performSmartWarp(op, &progress);

    CHECK(progress == 1);
    CHECK(res.output_image.width() == 22);
    CHECK(res.output_image.height() == 12);
    checkWarpedGradient(res, 3, true);
}

TEST_CASE("performSmartWarp rejects an empty image", "[projection][warp][smart_warp]")
{
    satdump::image::Image img;
    satdump::warp::WarpOperation op;
    op.input_image = &img;
    op.output_width = 720;
    op.output_height = 360;
    CHECK_THROWS(satdump::warp::performSmartWarp(op));
}
