#include "projection/utils/equirectangular.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using satdump::projection::EquirectangularProjection;

TEST_CASE("EquirectangularProjection forward maps a full globe to pixels", "[projection][utils][equirectangular]")
{
    EquirectangularProjection p;
    p.init(360, 180, -180, 90, 180, -90);

    struct Case
    {
        float lon, lat;
        int x, y;
    };
    const Case cases[] = {{-180, 90, 0, 0}, {0, 0, 180, 90}, {-179.5f, 89.5f, 0, 0}, {179.5f, -89.5f, 359, 179}, {45.2f, -30.7f, 225, 120}};
    for (auto &c : cases)
    {
        int x = -2, y = -2;
        p.forward(c.lon, c.lat, x, y);
        CHECK(x == c.x);
        CHECK(y == c.y);
    }
}

TEST_CASE("EquirectangularProjection forward rejects points outside the area", "[projection][utils][equirectangular]")
{
    EquirectangularProjection p;
    p.init(100, 50, 10, 60, 30, 50); // 20 x 10 degrees, 0.2 degree pixels

    int x, y;
    p.forward(20, 55, x, y);
    CHECK(x == 50);
    CHECK(y == 25);

    p.forward(9, 55, x, y);
    CHECK(x == -1);
    CHECK(y == -1);
    p.forward(20, 61, x, y);
    CHECK(x == -1);
    CHECK(y == -1);
    // The bottom-right corner itself is one pixel past the image
    p.forward(30, 50, x, y);
    CHECK(x == -1);
    CHECK(y == -1);

    // allow_oob extrapolates instead
    p.forward(32, 48, x, y, true);
    CHECK(x == 110);
    CHECK(y == 60);
}

TEST_CASE("EquirectangularProjection reverse maps pixels back to the area", "[projection][utils][equirectangular]")
{
    EquirectangularProjection p;
    p.init(100, 50, 10, 60, 30, 50);

    float lon, lat;
    p.reverse(0, 0, lon, lat);
    CHECK_THAT(lon, WithinAbs(10, 1e-4));
    CHECK_THAT(lat, WithinAbs(60, 1e-4));
    p.reverse(50, 25, lon, lat);
    CHECK_THAT(lon, WithinAbs(20, 1e-4));
    CHECK_THAT(lat, WithinAbs(55, 1e-4));
    p.reverse(99, 49, lon, lat);
    CHECK_THAT(lon, WithinAbs(29.8, 1e-4));
    CHECK_THAT(lat, WithinAbs(50.2, 1e-4));

    // Out of image pixels are reported as (-1, -1)
    p.reverse(100, 0, lon, lat);
    CHECK(lon == -1);
    CHECK(lat == -1);
    p.reverse(0, -1, lon, lat);
    CHECK(lon == -1);
    CHECK(lat == -1);
}

TEST_CASE("EquirectangularProjection forward/reverse round-trip on pixel centers", "[projection][utils][equirectangular]")
{
    EquirectangularProjection p;
    p.init(720, 360, -180, 90, 180, -90);
    for (int y = 0; y < 360; y += 37)
    {
        for (int x = 0; x < 720; x += 53)
        {
            float lon, lat;
            p.reverse(x, y, lon, lat);
            int ox, oy;
            p.forward(lon + 0.25f, lat - 0.25f, ox, oy); // Pixel center, avoids float edge effects
            CHECK(ox == x);
            CHECK(oy == y);
        }
    }
}
