// interpolator.h uses std::sort without including <algorithm>, so it has to come first.
// It is in its own block to keep clang-format from moving it.
#include <algorithm>

#include "common/tracking/interpolator.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;

TEST_CASE("NevilleInterpolator reproduces polynomials exactly", "[common][tracking][interpolator]")
{
    // Neville through n points is exact for polynomials of degree < n
    auto f = [](double x) { return 2 * x * x * x - 3 * x * x + x - 5; };
    std::vector<std::pair<double, double>> pts;
    for (double x : {-2.0, -0.5, 1.0, 2.5, 4.0})
        pts.push_back({x, f(x)});
    NevilleInterpolator interp(pts);
    for (double x = -3; x <= 5; x += 0.37)
        CHECK_THAT(interp.interpolate(x), WithinAbs(f(x), 1e-9));
}

TEST_CASE("NevilleInterpolator passes through its nodes regardless of input order", "[common][tracking][interpolator]")
{
    std::vector<std::pair<double, double>> pts = {{3, 9.5}, {0, 1}, {1, -2}, {2, 4}};
    NevilleInterpolator interp(pts);
    for (auto &p : pts)
        CHECK_THAT(interp.interpolate(p.first), WithinAbs(p.second, 1e-12));
}

TEST_CASE("NevilleInterpolator with a single point is constant", "[common][tracking][interpolator]")
{
    NevilleInterpolator interp({{1.0, 42.0}});
    CHECK(interp.interpolate(-100) == 42.0);
    CHECK(interp.interpolate(7) == 42.0);
}

TEST_CASE("LinearInterpolator sorts its points", "[common][tracking][interpolator]")
{
    LinearInterpolator interp({{3, 30}, {1, 10}, {2, 20}});
    auto pts = interp.get_points();
    REQUIRE(pts.size() == 3);
    CHECK(pts[0].first == 1);
    CHECK(pts[1].first == 2);
    CHECK(pts[2].first == 3);

    interp.set_points({{5, 0}, {-1, 1}, {2, 2}, {0, 3}});
    pts = interp.get_points();
    REQUIRE(pts.size() == 4);
    CHECK(std::is_sorted(pts.begin(), pts.end(), [](auto &a, auto &b) { return a.first < b.first; }));
}

TEST_CASE("LinearInterpolator is exact for linear data, including extrapolation", "[common][tracking][interpolator]")
{
    auto f = [](double x) { return 3.5 * x - 2; };
    std::vector<std::pair<double, double>> pts;
    for (double x : {0.0, 1.0, 2.5, 4.0, 10.0})
        pts.push_back({x, f(x)});
    LinearInterpolator interp(pts);
    for (double x = -2; x <= 12; x += 0.25)
        CHECK_THAT(interp.interpolate(x), WithinAbs(f(x), 1e-9));
}

TEST_CASE("LinearInterpolator hits every node after the first exactly", "[common][tracking][interpolator]")
{
    // The first node is affected by the segment selection bug below
    std::vector<std::pair<double, double>> pts = {{0, 0}, {1, 1}, {2, 0}, {3, 5}, {4, -1}};
    LinearInterpolator interp(pts);
    for (size_t i = 1; i < pts.size(); i++)
        CHECK_THAT(interp.interpolate(pts[i].first), WithinAbs(pts[i].second, 1e-12));
}

TEST_CASE("LinearInterpolator interpolates inside the last segment", "[common][tracking][interpolator]")
{
    LinearInterpolator interp({{0, 0}, {1, 1}, {2, 0}, {3, 4}});
    CHECK_THAT(interp.interpolate(2.5), WithinAbs(2, 1e-12));
}

// Bug: interpolator.h:63-77: start_pos ends up at the first node >= x and the segment used is
// [start_pos, start_pos + 1], i.e. the segment *after* x. Only the last segment (clamped by the
// "start_pos + 1 == size" branch) is right; elsewhere the value is extrapolated from the wrong
// pair of nodes. SatelliteTracker uses this for ephemeris-based positions.
TEST_CASE("LinearInterpolator interpolates between the two surrounding nodes", "[common][tracking][interpolator][!shouldfail]")
{
    LinearInterpolator interp({{0, 0}, {1, 1}, {2, 0}, {3, 4}});
    CHECK_THAT(interp.interpolate(0.5), WithinAbs(0.5, 1e-12));
    CHECK_THAT(interp.interpolate(1.5), WithinAbs(0.5, 1e-12));
    CHECK_THAT(interp.interpolate(0.0), WithinAbs(0.0, 1e-12)); // comes back as 2
}
