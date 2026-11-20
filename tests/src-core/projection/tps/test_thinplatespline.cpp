#include "projection/thinplatespline.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;
using satdump::projection::VizGeorefSpline2D;

namespace
{
    // Affine map used as ground truth: (u, v) = (2x - 0.5y + 3, 0.25x + 1.5y - 7)
    void affine(double x, double y, double out[2])
    {
        out[0] = 2 * x - 0.5 * y + 3;
        out[1] = 0.25 * x + 1.5 * y - 7;
    }

    // Smooth non-linear field for control-point tests
    void wavy(double x, double y, double out[2])
    {
        out[0] = std::sin(x * 0.3) * 10 + y;
        out[1] = std::cos(y * 0.2) * 5 + x * x * 0.01;
    }
} // namespace

TEST_CASE("VizGeorefSpline2D solve reports the interpolation type", "[projection][tps]")
{
    double v[2] = {1, 2};

    VizGeorefSpline2D none(2);
    CHECK(none.solve() == 0);

    VizGeorefSpline2D one(2);
    one.add_point(0, 0, v);
    CHECK(one.solve() == 1);

    VizGeorefSpline2D two(2);
    two.add_point(0, 0, v);
    two.add_point(1, 1, v);
    CHECK(two.solve() == 2);

    VizGeorefSpline2D line(2); // Collinear (diagonal) points are a 1D problem
    for (int i = 0; i < 5; i++)
        line.add_point(i, 2 * i, v);
    CHECK(line.solve() == 3);

    VizGeorefSpline2D full(2);
    full.add_point(0, 0, v);
    full.add_point(1, 0, v);
    full.add_point(0, 1, v);
    full.add_point(1, 1.5, v);
    CHECK(full.solve() == 4);

    // Points all on a horizontal line are degenerate in GDAL's formulation
    VizGeorefSpline2D flat(2);
    for (int i = 0; i < 4; i++)
        flat.add_point(i, 5, v);
    CHECK(flat.solve() == 0);
}

TEST_CASE("VizGeorefSpline2D reproduces its control points exactly", "[projection][tps]")
{
    VizGeorefSpline2D tps(2);
    std::vector<std::pair<double, double>> pts;
    for (int y = 0; y < 6; y++)
        for (int x = 0; x < 7; x++)
            pts.push_back({x * 3.0 + (y % 2) * 0.7, y * 2.5 + (x % 3) * 0.3}); // Irregular grid

    for (auto &p : pts)
    {
        double v[2];
        wavy(p.first, p.second, v);
        REQUIRE(tps.add_point(p.first, p.second, v));
    }
    REQUIRE(tps.solve() == 4);
    CHECK(tps._nof_points == (int)pts.size());

    for (auto &p : pts)
    {
        double expected[2], got[2] = {0, 0};
        wavy(p.first, p.second, expected);
        REQUIRE(tps.get_point(p.first, p.second, got) == 1);
        CHECK_THAT(got[0], WithinAbs(expected[0], 1e-8));
        CHECK_THAT(got[1], WithinAbs(expected[1], 1e-8));
    }
}

TEST_CASE("VizGeorefSpline2D interpolates an affine map exactly everywhere", "[projection][tps]")
{
    VizGeorefSpline2D tps(2);
    const double pts[][2] = {{0, 0}, {10, 0}, {0, 10}, {10, 10}, {3, 7}, {8, 2}, {5, 5}, {-4, 6}};
    for (auto &p : pts)
    {
        double v[2];
        affine(p[0], p[1], v);
        tps.add_point(p[0], p[1], v);
    }
    REQUIRE(tps.solve() == 4);

    // The TPS includes an affine part, so the radial weights vanish for affine data
    for (double y = -5; y <= 15; y += 2.5)
    {
        for (double x = -5; x <= 15; x += 2.5)
        {
            double expected[2], got[2];
            affine(x, y, expected);
            tps.get_point(x, y, got);
            CHECK_THAT(got[0], WithinAbs(expected[0], 1e-8));
            CHECK_THAT(got[1], WithinAbs(expected[1], 1e-8));
        }
    }
}

TEST_CASE("VizGeorefSpline2D degenerate cases interpolate sensibly", "[projection][tps]")
{
    SECTION("one point is a constant")
    {
        VizGeorefSpline2D tps(1);
        double v = 42;
        tps.add_point(3, 4, &v);
        tps.solve();
        double out = 0;
        tps.get_point(-100, 55, &out);
        CHECK(out == 42);
    }

    SECTION("two points are linear along their axis")
    {
        VizGeorefSpline2D tps(1);
        double v0 = 10, v1 = 30;
        tps.add_point(0, 0, &v0);
        tps.add_point(4, 0, &v1);
        tps.solve();
        double out = 0;
        tps.get_point(1, 0, &out);
        CHECK_THAT(out, WithinAbs(15, 1e-12));
        tps.get_point(8, 0, &out); // Extrapolates
        CHECK_THAT(out, WithinAbs(50, 1e-12));
        tps.get_point(2, 9, &out); // Only the projection on the axis matters
        CHECK_THAT(out, WithinAbs(20, 1e-12));
    }

    SECTION("collinear points are piecewise linear")
    {
        VizGeorefSpline2D tps(1);
        const double u[] = {0, 1, 3, 6};
        const double val[] = {0, 10, 0, 30};
        for (int i = 0; i < 4; i++)
            tps.add_point(u[i], u[i], &val[i]); // Added along the diagonal, like ChannelTransform does
        REQUIRE(tps.solve() == 3);

        double out = 0;
        tps.get_point(0.5, 0.5, &out);
        CHECK_THAT(out, WithinAbs(5, 1e-9));
        tps.get_point(2, 2, &out);
        CHECK_THAT(out, WithinAbs(5, 1e-9));
        tps.get_point(4.5, 4.5, &out);
        CHECK_THAT(out, WithinAbs(15, 1e-9));
        tps.get_point(3, 3, &out);
        CHECK_THAT(out, WithinAbs(0, 1e-9));
    }
}

TEST_CASE("VizGeorefSpline2D returns zeros after a point is added without re-solving", "[projection][tps]")
{
    VizGeorefSpline2D tps(2);
    double v[2] = {1, 2};
    tps.add_point(0, 0, v);
    tps.add_point(1, 0, v);
    tps.add_point(0, 1, v);
    REQUIRE(tps.solve() == 4);
    tps.add_point(5, 5, v);

    double out[2] = {9, 9};
    CHECK(tps.get_point(0, 0, out) == 0);
    CHECK(out[0] == 0);
    CHECK(out[1] == 0);

    // Solving again brings it back
    REQUIRE(tps.solve() == 4);
    tps.get_point(0, 0, out);
    CHECK_THAT(out[0], WithinAbs(1, 1e-9));
    CHECK_THAT(out[1], WithinAbs(2, 1e-9));
}

TEST_CASE("VizGeorefSpline2D grows past its initial capacity", "[projection][tps]")
{
    VizGeorefSpline2D tps(2);
    const int n = 400; // Several times the initial allocation
    for (int i = 0; i < n; i++)
    {
        double x = i % 20, y = i / 20;
        double v[2];
        affine(x, y, v);
        REQUIRE(tps.add_point(x, y, v));
    }
    REQUIRE(tps.solve() == 4);
    CHECK(tps._nof_points == n);
    CHECK(tps._max_nof_points >= n);

    double expected[2], got[2];
    affine(7.5, 12.25, expected);
    tps.get_point(7.5, 12.25, got);
    CHECK_THAT(got[0], WithinAbs(expected[0], 1e-6));
    CHECK_THAT(got[1], WithinAbs(expected[1], 1e-6));
}
