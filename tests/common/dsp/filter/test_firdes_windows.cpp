#include "common/dsp/filter/firdes.h"
#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cmath>
#include <numeric>
#include <stdexcept>

using Catch::Approx;
using dsp::fft::window;

namespace
{
    void checkSymmetric(const std::vector<float> &w, float margin = 1e-6f)
    {
        for (size_t i = 0; i < w.size() / 2; i++)
            REQUIRE(w[i] == Approx(w[w.size() - 1 - i]).margin(margin));
    }
}

TEST_CASE("window::rectangular is all ones", "[common][dsp][firdes][window]")
{
    auto w = window::rectangular(17);
    REQUIRE(w.size() == 17);
    for (float v : w)
        CHECK(v == 1.0f);
}

TEST_CASE("window::hamming endpoints, peak and symmetry", "[common][dsp][firdes][window]")
{
    auto w = window::hamming(51);
    REQUIRE(w.size() == 51);
    CHECK(w.front() == Approx(0.08f));
    CHECK(w.back() == Approx(0.08f));
    CHECK(w[25] == Approx(1.0f));
    checkSymmetric(w);

    // Closed form 0.54 - 0.46 cos(2 pi n / (N - 1))
    for (int n = 0; n < 51; n++)
        CHECK(w[n] == Approx(0.54 - 0.46 * cos(2 * M_PI * n / 50.0)).margin(1e-6));
}

TEST_CASE("window::hann endpoints, peak and symmetry", "[common][dsp][firdes][window]")
{
    auto w = window::hann(33);
    CHECK(w.front() == Approx(0.0f).margin(1e-7));
    CHECK(w.back() == Approx(0.0f).margin(1e-7));
    CHECK(w[16] == Approx(1.0f));
    CHECK(w[8] == Approx(0.5f).margin(1e-6)); // Quarter way up the cosine
    checkSymmetric(w);
}

TEST_CASE("window::blackman endpoints, peak and symmetry", "[common][dsp][firdes][window]")
{
    auto w = window::blackman(41);
    CHECK(w.front() == Approx(0.0f).margin(1e-6));
    CHECK(w.back() == Approx(0.0f).margin(1e-6));
    CHECK(w[20] == Approx(1.0f));
    checkSymmetric(w);
}

TEST_CASE("window::blackman_harris supports the 4 attenuation variants", "[common][dsp][firdes][window]")
{
    // Coefficient sets from the source, peak = c0 + c1 + c2 (+ c3)
    struct Case
    {
        int atten;
        double peak;
        double edge;
    };
    const Case cases[] = {
        {61, 0.42323 + 0.49755 + 0.07922, 0.42323 - 0.49755 + 0.07922},
        {67, 0.44959 + 0.49364 + 0.05677, 0.44959 - 0.49364 + 0.05677},
        {74, 0.40271 + 0.49703 + 0.09392 + 0.00183, 0.40271 - 0.49703 + 0.09392 - 0.00183},
        {92, 0.35875 + 0.48829 + 0.14128 + 0.01168, 0.35875 - 0.48829 + 0.14128 - 0.01168},
    };

    for (const auto &c : cases)
    {
        INFO("atten = " << c.atten);
        auto w = window::blackman_harris(31, c.atten);
        REQUIRE(w.size() == 31);
        CHECK(w[15] == Approx(c.peak).margin(1e-5));
        CHECK(w[0] == Approx(c.edge).margin(1e-5));
        checkSymmetric(w);
    }

    CHECK_THROWS_AS(window::blackman_harris(31, 50), std::out_of_range);
}

TEST_CASE("window::kaiser shape and beta handling", "[common][dsp][firdes][window]")
{
    // beta = 0 degenerates to a rectangular window
    auto flat = window::kaiser(21, 0.0);
    for (float v : flat)
        CHECK(v == Approx(1.0f));

    // Edges are 1 / I0(beta), I0(5) = 27.2398718
    auto w = window::kaiser(21, 5.0);
    CHECK(w.front() == Approx(1.0 / 27.2398718).epsilon(1e-5));
    CHECK(w.back() == Approx(1.0 / 27.2398718).epsilon(1e-5));
    CHECK(w[10] == Approx(1.0f));
    checkSymmetric(w);

    // Monotonic increase up to the center
    for (int i = 1; i <= 10; i++)
        CHECK(w[i] > w[i - 1]);

    // Larger beta = narrower window
    auto narrow = window::kaiser(21, 10.0);
    CHECK(narrow[3] < w[3]);

    CHECK_THROWS_AS(window::kaiser(21, -1.0), std::out_of_range);
}

TEST_CASE("window::bartlett is triangular", "[common][dsp][firdes][window]")
{
    auto w = window::bartlett(11);
    for (int n = 0; n < 11; n++)
        CHECK(w[n] == Approx(1.0 - fabs(n - 5.0) / 5.0).margin(1e-6));

    auto e = window::bartlett(10);
    checkSymmetric(e);
    CHECK(e[0] == 0.0f);
}

TEST_CASE("window::flattop peak and symmetry", "[common][dsp][firdes][window]")
{
    auto w = window::flattop(65);
    // Peak = (1 + 1.93 + 1.29 + 0.388 + 0.028) / 4.63867
    CHECK(w[32] == Approx((1 + 1.93 + 1.29 + 0.388 + 0.028) / 4.63867).epsilon(1e-5));
    checkSymmetric(w, 1e-5f);
    // Flat-top windows go slightly negative
    CHECK(*std::min_element(w.begin(), w.end()) < 0.0f);
}

TEST_CASE("window::coswindow matches the generic cosine sum", "[common][dsp][firdes][window]")
{
    auto w3 = window::coswindow(25, 0.5, 0.3, 0.2);
    auto w4 = window::coswindow(25, 0.5, 0.3, 0.2, 0.1);
    auto w5 = window::coswindow(25, 0.5, 0.3, 0.2, 0.1, 0.05);
    for (int n = 0; n < 25; n++)
    {
        double x = 2 * M_PI * n / 24.0;
        double e3 = 0.5 - 0.3 * cos(x) + 0.2 * cos(2 * x);
        double e4 = e3 - 0.1 * cos(3 * x);
        double e5 = e4 + 0.05 * cos(4 * x);
        CHECK(w3[n] == Approx(e3).margin(1e-5));
        CHECK(w4[n] == Approx(e4).margin(1e-5));
        CHECK(w5[n] == Approx(e5).margin(1e-5));
    }
}

TEST_CASE("window::max_attenuation table", "[common][dsp][firdes][window]")
{
    CHECK(window::max_attenuation(window::WIN_HAMMING) == 53);
    CHECK(window::max_attenuation(window::WIN_HANN) == 44);
    CHECK(window::max_attenuation(window::WIN_BLACKMAN) == 74);
    CHECK(window::max_attenuation(window::WIN_RECTANGULAR) == 21);
    CHECK(window::max_attenuation(window::WIN_BLACKMAN_HARRIS) == 92);
    CHECK(window::max_attenuation(window::WIN_BARTLETT) == 27);
    CHECK(window::max_attenuation(window::WIN_FLATTOP) == 93);
    // Kaiser: beta = 0.1102 (A - 8.7)
    CHECK(window::max_attenuation(window::WIN_KAISER, 6.76) == Approx(6.76 / 0.1102 + 8.7));
    CHECK_THROWS_AS(window::max_attenuation(window::WIN_NONE), std::out_of_range);
}

TEST_CASE("window::build dispatches to each generator", "[common][dsp][firdes][window]")
{
    const int n = 37;
    CHECK(window::build(window::WIN_RECTANGULAR, n, 0) == window::rectangular(n));
    CHECK(window::build(window::WIN_HAMMING, n, 0) == window::hamming(n));
    CHECK(window::build(window::WIN_HANN, n, 0) == window::hann(n));
    CHECK(window::build(window::WIN_BLACKMAN, n, 0) == window::blackman(n));
    CHECK(window::build(window::WIN_BLACKMAN_HARRIS, n, 0) == window::blackman_harris(n));
    CHECK(window::build(window::WIN_KAISER, n, 4.0) == window::kaiser(n, 4.0));
    CHECK(window::build(window::WIN_BARTLETT, n, 0) == window::bartlett(n));
    CHECK(window::build(window::WIN_FLATTOP, n, 0) == window::flattop(n));
    CHECK_THROWS_AS(window::build(window::WIN_NONE, n, 0), std::out_of_range);
}

TEST_CASE("window::build normalize gives unit RMS", "[common][dsp][firdes][window]")
{
    auto type = GENERATE(window::WIN_HAMMING, window::WIN_HANN, window::WIN_BLACKMAN, window::WIN_KAISER);
    INFO("type = " << (int)type);
    auto raw = window::build(type, 64, 6.76, false);
    auto w = window::build(type, 64, 6.76, true);
    REQUIRE(w.size() == 64);

    double pwr = 0;
    for (float v : w)
        pwr += v * v;
    CHECK(sqrt(pwr / w.size()) == Approx(1.0).epsilon(1e-5));

    // Same shape, only scaled
    for (int i = 1; i < 64; i++)
        CHECK(w[i] * raw[0] == Approx(w[0] * raw[i]).margin(1e-5));
}
