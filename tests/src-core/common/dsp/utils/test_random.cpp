#include "common/dsp/utils/random.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <set>
#include <vector>

using Catch::Approx;

namespace
{
    template <typename F>
    void meanVar(F gen, int n, double &mean, double &var)
    {
        double s = 0, s2 = 0;
        for (int i = 0; i < n; i++)
        {
            double v = gen();
            s += v;
            s2 += v * v;
        }
        mean = s / n;
        var = s2 / n - mean * mean;
    }
} // namespace

TEST_CASE("Random is deterministic for a given non-zero seed", "[common][dsp][random]")
{
    dsp::Random a(1234), b(1234), c(4321);
    bool differs = false;
    for (int i = 0; i < 100; i++)
    {
        float va = a.ran1(), vb = b.ran1(), vc = c.ran1();
        REQUIRE(va == vb);
        differs |= va != vc;
    }
    CHECK(differs);

    // Reseeding restarts the uniform sequence
    dsp::Random d(77);
    std::vector<float> first;
    for (int i = 0; i < 10; i++)
        first.push_back(d.ran1());
    d.reseed(77);
    for (int i = 0; i < 10; i++)
        CHECK(d.ran1() == first[i]);

    // Same seed, same gaussian sequence
    dsp::Random e(77), f(77);
    for (int i = 0; i < 10; i++)
        CHECK(e.gasdev() == f.gasdev());
}

TEST_CASE("Random::ran1 is uniform over the half-open range 0 to 1", "[common][dsp][random]")
{
    dsp::Random r(1);
    double mean, var;
    float lo = 1, hi = 0;
    meanVar(
        [&]()
        {
            float v = r.ran1();
            lo = std::min(lo, v);
            hi = std::max(hi, v);
            return v;
        },
        200000, mean, var);
    CHECK(lo >= 0.0f);
    CHECK(hi < 1.0f);
    CHECK(mean == Approx(0.5).margin(0.005));
    CHECK(var == Approx(1.0 / 12).margin(0.002));
}

TEST_CASE("Random::ran_int respects the half-open integer range", "[common][dsp][random]")
{
    dsp::Random r(2, -3, 4);
    std::set<int> seen;
    size_t mismatches = 0, first = 0;
    for (int i = 0; i < 10000; i++)
    {
        int v = r.ran_int();
        if ((!(v >= -3) || !(v < 4)) && mismatches++ == 0)
            first = i;
        seen.insert(v);
    }
    INFO("first out of range draw " << first);
    REQUIRE(mismatches == 0);
    CHECK(seen.size() == 7);

    // Default range is [0, 2): random bits
    dsp::Random bits(3);
    size_t bit_mismatches = 0, bit_first = 0;
    for (int i = 0; i < 1000; i++)
    {
        int v = bits.ran_int();
        if (!(v == 0 || v == 1) && bit_mismatches++ == 0)
            bit_first = i;
    }
    INFO("first non-bit draw " << bit_first);
    REQUIRE(bit_mismatches == 0);

    r.set_integer_limits(10, 11);
    for (int i = 0; i < 100; i++)
        REQUIRE(r.ran_int() == 10);
}

TEST_CASE("Random::gasdev is standard normal", "[common][dsp][random]")
{
    dsp::Random r(5);
    double mean, var;
    meanVar([&]() { return r.gasdev(); }, 200000, mean, var);
    CHECK(mean == Approx(0.0).margin(0.01));
    CHECK(var == Approx(1.0).margin(0.02));

    // About 68.3 % within one sigma
    int inside = 0;
    for (int i = 0; i < 100000; i++)
        inside += fabs(r.gasdev()) < 1.0f;
    CHECK(inside / 100000.0 == Approx(0.6827).margin(0.01));
}

TEST_CASE("Random::rayleigh_complex has unit-variance components", "[common][dsp][random]")
{
    dsp::Random r(6);
    double sr = 0, si = 0, sr2 = 0, si2 = 0;
    const int n = 100000;
    for (int i = 0; i < n; i++)
    {
        complex_t c = r.rayleigh_complex();
        sr += c.real;
        si += c.imag;
        sr2 += c.real * c.real;
        si2 += c.imag * c.imag;
    }
    CHECK(sr / n == Approx(0.0).margin(0.015));
    CHECK(si / n == Approx(0.0).margin(0.015));
    CHECK(sr2 / n == Approx(1.0).margin(0.03));
    CHECK(si2 / n == Approx(1.0).margin(0.03));
}

TEST_CASE("Random::rayleigh has the Rayleigh(1) mean", "[common][dsp][random]")
{
    dsp::Random r(7);
    double mean, var;
    meanVar([&]() { return r.rayleigh(); }, 200000, mean, var);
    CHECK(mean == Approx(sqrt(M_PI / 2)).margin(0.01));
    CHECK(var == Approx((4 - M_PI) / 2).margin(0.01));
}

TEST_CASE("Random::laplacian is a zero-mean Laplace(1) distribution", "[common][dsp][random]")
{
    dsp::Random r(8);
    double mean, var;
    meanVar([&]() { return r.laplacian(); }, 200000, mean, var);
    CHECK(mean == Approx(0.0).margin(0.02));
    // Scale 1 Laplace has variance 2 (the header comment says 1, as in GNU Radio)
    CHECK(var == Approx(2.0).margin(0.05));
}

TEST_CASE("Random::impulse is zero below the factor", "[common][dsp][random]")
{
    dsp::Random r(9);
    int nonzero = 0;
    const int n = 200000;
    for (int i = 0; i < n; i++)
    {
        float v = r.impulse(5);
        if (v != 0.0f)
        {
            REQUIRE(v > 5.0f);
            nonzero++;
        }
    }
    // P(sqrt(2) * Exp(1) > 5) = exp(-5 / sqrt(2))
    CHECK(nonzero / (double)n == Approx(exp(-5 / sqrt(2.0))).margin(0.003));
}
