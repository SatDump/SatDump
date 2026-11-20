#include "dsp/fast_math.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using satdump::ndsp::fast_invsqrt;

TEST_CASE("ndsp fast_invsqrt approximates 1/sqrt(x)", "[dsp][fast_math]")
{
    // One Newton iteration after the magic constant guess : max relative error is ~0.175%
    for (float x : {1e-6f, 0.01f, 0.25f, 1.0f, 2.0f, 3.0f, 10.0f, 1234.5f, 1e6f, 1e20f})
    {
        float expected = 1.0f / std::sqrt(x);
        float got = fast_invsqrt(x);
        INFO("x = " << x);
        CHECK(std::fabs(got - expected) / expected < 0.0018f);
    }
}

TEST_CASE("ndsp fast_invsqrt error stays bounded over a sweep", "[dsp][fast_math]")
{
    double worst = 0;
    for (int i = 1; i <= 20000; i++)
    {
        float x = i * 0.0137f;
        double rel = std::fabs(fast_invsqrt(x) * std::sqrt((double)x) - 1.0);
        worst = std::max(worst, rel);
    }
    CHECK(worst < 0.0018);
    // The approximation is not exact either
    CHECK(worst > 1e-4);
}
