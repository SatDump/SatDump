#include "common/dsp/utils/fast_trig.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using Catch::Approx;

TEST_CASE("fast_atan2f matches atan2 in every quadrant", "[common][dsp][fast_trig]")
{
    double max_err = 0;
    for (int i = 0; i < 3600; i++)
    {
        double a = -M_PI + (i + 0.5) * (2 * M_PI / 3600);
        for (float r : {1e-3f, 1.0f, 250.0f})
        {
            float y = r * sin(a), x = r * cos(a);
            max_err = std::max(max_err, (double)fabs(dsp::fast_atan2f(y, x) - atan2f(y, x)));
        }
    }
    // Table + linear interpolation is accurate to a few 1e-6 rad
    CHECK(max_err < 1e-5);
}

TEST_CASE("fast_atan2f special values", "[common][dsp][fast_trig]")
{
    CHECK(dsp::fast_atan2f(0.0f, 0.0f) == 0.0f);
    CHECK(dsp::fast_atan2f(0.0f, 1.0f) == Approx(0.0f).margin(1e-7));
    CHECK(dsp::fast_atan2f(1.0f, 0.0f) == Approx(M_PI / 2));
    CHECK(dsp::fast_atan2f(-1.0f, 0.0f) == Approx(-M_PI / 2));
    CHECK(dsp::fast_atan2f(0.0f, -1.0f) == Approx(M_PI));
    CHECK(dsp::fast_atan2f(1.0f, 1.0f) == Approx(M_PI / 4).epsilon(1e-5));
    CHECK(dsp::fast_atan2f(-1.0f, -1.0f) == Approx(-3 * M_PI / 4).epsilon(1e-5));
    // Small-angle branch
    CHECK(dsp::fast_atan2f(1e-4f, 1.0f) == Approx(1e-4f).epsilon(1e-3));
    // Scale invariant
    CHECK(dsp::fast_atan2f(3.0f, 4.0f) == Approx(dsp::fast_atan2f(3e-6f, 4e-6f)).margin(1e-7));
}

TEST_CASE("fast_sin and fast_cos approximate sin and cos over [-pi, pi]", "[common][dsp][fast_trig]")
{
    double max_sin = 0, max_cos = 0, half_sin = 0, half_cos = 0;
    for (int i = 0; i <= 2000; i++)
    {
        float x = -M_PI + i * (2 * M_PI / 2000);
        double es = fabs(dsp::fast_sin(x) - sinf(x));
        double ec = fabs(dsp::fast_cos(x) - cosf(x));
        max_sin = std::max(max_sin, es);
        max_cos = std::max(max_cos, ec);
        if (fabs(x) <= M_PI / 2)
        {
            half_sin = std::max(half_sin, es);
            half_cos = std::max(half_cos, ec);
        }
    }
    // Truncated polynomials: very good over [-pi/2, pi/2], a few 1e-3 off near +-pi
    CHECK(half_sin < 1e-4);
    CHECK(half_cos < 1e-4);
    CHECK(max_sin < 1e-2);
    CHECK(max_cos < 5e-3);

    // Very accurate close to 0
    for (float x = -0.5f; x <= 0.5f; x += 0.01f)
    {
        CHECK(dsp::fast_sin(x) == Approx(sinf(x)).margin(1e-6));
        CHECK(dsp::fast_cos(x) == Approx(cosf(x)).margin(1e-6));
    }

    // Odd / even symmetry
    CHECK(dsp::fast_sin(-1.234f) == -dsp::fast_sin(1.234f));
    CHECK(dsp::fast_cos(-1.234f) == dsp::fast_cos(1.234f));
    CHECK(dsp::fast_sin(0.0f) == 0.0f);
    CHECK(dsp::fast_cos(0.0f) == 1.0f);
}
