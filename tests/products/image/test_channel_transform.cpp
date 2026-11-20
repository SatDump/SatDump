#include "products/image/channel_transform.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using satdump::ChannelTransform;

namespace
{
    void fwd(const ChannelTransform &t, double x, double y, double &ox, double &oy)
    {
        ox = x, oy = y;
        t.forward(&ox, &oy);
    }

    void rev(const ChannelTransform &t, double x, double y, double &ox, double &oy)
    {
        ox = x, oy = y;
        t.reverse(&ox, &oy);
    }

    void checkRoundTrip(const ChannelTransform &t, double tol)
    {
        for (double y = 0; y <= 50; y += 12.5)
        {
            for (double x = 0; x <= 100; x += 12.5)
            {
                double fx, fy, rx, ry;
                fwd(t, x, y, fx, fy);
                rev(t, fx, fy, rx, ry);
                CHECK_THAT(rx, WithinAbs(x, tol));
                CHECK_THAT(ry, WithinAbs(y, tol));
            }
        }
    }

    ChannelTransform viaJson(const ChannelTransform &t)
    {
        nlohmann::json j = t;
        return j.get<ChannelTransform>();
    }
} // namespace

TEST_CASE("ChannelTransform is invalid until initialised", "[products][channel_transform]")
{
    ChannelTransform t;
    CHECK(t.getType() == ChannelTransform::TYPE_INVALID);
    double x = 1, y = 2;
    CHECK_THROWS(t.forward(&x, &y));
    CHECK_THROWS(t.reverse(&x, &y));
}

TEST_CASE("ChannelTransform none is the identity", "[products][channel_transform]")
{
    ChannelTransform t;
    t.init_none();
    CHECK(t.getType() == ChannelTransform::TYPE_NONE);
    double x, y;
    fwd(t, 12.5, -3, x, y);
    CHECK(x == 12.5);
    CHECK(y == -3);
    rev(t, 7, 8, x, y);
    CHECK(x == 7);
    CHECK(y == 8);
}

TEST_CASE("ChannelTransform affine maps channel pixels to reference pixels", "[products][channel_transform]")
{
    ChannelTransform t;
    t.init_affine(4, 2, 1.5, -3);
    CHECK(t.getType() == ChannelTransform::TYPE_AFFINE);

    double x, y;
    fwd(t, 10, 20, x, y);
    CHECK_THAT(x, WithinAbs(41.5, 1e-12));
    CHECK_THAT(y, WithinAbs(37, 1e-12));
    rev(t, 41.5, 37, x, y);
    CHECK_THAT(x, WithinAbs(10, 1e-12));
    CHECK_THAT(y, WithinAbs(20, 1e-12));
    checkRoundTrip(t, 1e-12);
}

TEST_CASE("ChannelTransform affine_slantx shears y along x before scaling", "[products][channel_transform]")
{
    ChannelTransform t;
    t.init_affine_slantx(2, 3, 1, -1, 4, 0.5);
    CHECK(t.getType() == ChannelTransform::TYPE_AFFINE_SLANTX);

    // y' = (y - (x - 4) * 0.5) * 3 - 1, x' = 2x + 1
    double x, y;
    fwd(t, 10, 5, x, y);
    CHECK_THAT(x, WithinAbs(21, 1e-12));
    CHECK_THAT(y, WithinAbs(5, 1e-12));
    fwd(t, 4, 5, x, y); // No shear at the slant center
    CHECK_THAT(y, WithinAbs(14, 1e-12));
    rev(t, 21, 5, x, y);
    CHECK_THAT(x, WithinAbs(10, 1e-12));
    CHECK_THAT(y, WithinAbs(5, 1e-12));
    checkRoundTrip(t, 1e-12);
}

TEST_CASE("ChannelTransform affine_interpx applies a piecewise-linear x correction", "[products][channel_transform]")
{
    ChannelTransform t;
    t.init_affine_interpx(1, 2, 0, 0, {{0, 0}, {50, 100}, {100, 150}});
    CHECK(t.getType() == ChannelTransform::TYPE_AFFINE_INTERPX);

    double x, y;
    fwd(t, 25, 3, x, y);
    CHECK_THAT(x, WithinAbs(50, 1e-9));
    CHECK_THAT(y, WithinAbs(6, 1e-12));
    fwd(t, 75, 0, x, y);
    CHECK_THAT(x, WithinAbs(125, 1e-9));
    rev(t, 125, 6, x, y);
    CHECK_THAT(x, WithinAbs(75, 1e-9));
    CHECK_THAT(y, WithinAbs(3, 1e-12));
    checkRoundTrip(t, 1e-9);
}

TEST_CASE("ChannelTransform JSON round-trip preserves the transform", "[products][channel_transform]")
{
    ChannelTransform none, affine, slant, interp;
    none.init_none();
    affine.init_affine(0.5, 0.25, 10, 20);
    slant.init_affine_slantx(2, 1, 3, 4, 50, 0.1);
    interp.init_affine_interpx(2, 1, 1, 0, {{0, 0}, {100, 120}, {200, 210}});

    for (ChannelTransform *t : {&none, &affine, &slant, &interp})
    {
        ChannelTransform t2 = viaJson(*t);
        CHECK(t2.getType() == t->getType());
        CHECK(nlohmann::json(t2) == nlohmann::json(*t));
        for (double x : {0.0, 33.0, 99.5})
        {
            double ax, ay, bx, by;
            fwd(*t, x, 7, ax, ay);
            fwd(t2, x, 7, bx, by);
            CHECK_THAT(bx, WithinAbs(ax, 1e-9));
            CHECK_THAT(by, WithinAbs(ay, 1e-9));
        }
    }

    nlohmann::json j = affine;
    CHECK(j["type"] == (int)ChannelTransform::TYPE_AFFINE);
    CHECK(j["ax"] == 0.5);
    CHECK(j["ay"] == 0.25);
    CHECK(j["bx"] == 10);
    CHECK(j["by"] == 20);
    CHECK(nlohmann::json(none) == nlohmann::json{{"type", (int)ChannelTransform::TYPE_NONE}});
}

TEST_CASE("ChannelTransform interp_xy follows a 2D TPS through its points", "[products][channel_transform]")
{
    // Note: init_interp_xy() and reverse() print debug output to stdout
    std::vector<std::pair<std::pair<double, double>, std::pair<double, double>>> pts;
    for (double y : {0.0, 10.0, 20.0})
        for (double x : {0.0, 10.0, 20.0})
            pts.push_back({{x, y}, {2 * x + 0.5 * y + 1, -x + 3 * y}}); // Affine, so the TPS is exact everywhere

    ChannelTransform t;
    t.init_interp_xy(pts);
    CHECK(t.getType() == ChannelTransform::TYPE_INTERP_XY);

    double x, y;
    fwd(t, 5, 15, x, y);
    CHECK_THAT(x, WithinAbs(2 * 5 + 0.5 * 15 + 1, 1e-8));
    CHECK_THAT(y, WithinAbs(-5 + 3 * 15, 1e-8));
    rev(t, x, y, x, y);
    CHECK_THAT(x, WithinAbs(5, 1e-8));
    CHECK_THAT(y, WithinAbs(15, 1e-8));
}
