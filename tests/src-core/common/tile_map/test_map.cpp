#include "common/tile_map/map.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <utility>

// Internal helpers of tile_map/map.cpp: not in the header, but have external linkage
std::pair<double, double> deg2num(double lat, double lon, int zoom);
std::pair<double, double> from4326_to3857(double lat, double lon);

namespace
{
    const double WEBMERC_MAX_LAT = 85.0511287798066;
    const double WEBMERC_HALF_WIDTH = M_PI * 6378137.0; // 20037508.34 m
} // namespace

TEST_CASE("deg2num maps lat/lon to slippy map tile coordinates", "[common][tile_map]")
{
    auto c0 = deg2num(0, 0, 0);
    CHECK(c0.first == Catch::Approx(0.5));
    CHECK(c0.second == Catch::Approx(0.5));

    auto c1 = deg2num(0, 0, 1);
    CHECK(c1.first == Catch::Approx(1.0));
    CHECK(c1.second == Catch::Approx(1.0));

    // Edges of the world at zoom 3 (8x8 tiles)
    CHECK(deg2num(0, -180, 3).first == Catch::Approx(0.0).margin(1e-12));
    CHECK(deg2num(0, 180, 3).first == Catch::Approx(8.0));
    CHECK(deg2num(WEBMERC_MAX_LAT, 0, 3).second == Catch::Approx(0.0).margin(1e-9));
    CHECK(deg2num(-WEBMERC_MAX_LAT, 0, 3).second == Catch::Approx(8.0));

    // Northern latitudes have smaller y; east longitudes have larger x
    auto paris = deg2num(48.8566, 2.3522, 10);
    CHECK(std::floor(paris.first) == 518);
    CHECK(std::floor(paris.second) == 352);
}

TEST_CASE("from4326_to3857 projects to Web Mercator meters", "[common][tile_map]")
{
    auto o = from4326_to3857(0, 0);
    CHECK(o.first == Catch::Approx(0.0).margin(1e-6));
    CHECK(o.second == Catch::Approx(0.0).margin(1e-6));

    CHECK(from4326_to3857(0, 180).first == Catch::Approx(WEBMERC_HALF_WIDTH).epsilon(1e-12));
    CHECK(from4326_to3857(0, -180).first == Catch::Approx(-WEBMERC_HALF_WIDTH).epsilon(1e-12));
    CHECK(from4326_to3857(WEBMERC_MAX_LAT, 0).second == Catch::Approx(WEBMERC_HALF_WIDTH).epsilon(1e-9));
    CHECK(from4326_to3857(-WEBMERC_MAX_LAT, 0).second == Catch::Approx(-WEBMERC_HALF_WIDTH).epsilon(1e-9));

    // Symmetric about the equator
    CHECK(from4326_to3857(45, 10).second == Catch::Approx(-from4326_to3857(-45, 10).second).epsilon(1e-12));
}
