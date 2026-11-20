#include "common/dsp/window/window.h"
#include "../dsp_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>
#include <numeric>

using Catch::Approx;

TEST_CASE("window::cosine evaluates an alternating cosine sum", "[common][dsp][window]")
{
    const double coefs[] = {0.5, 0.3, 0.2};
    for (double n = 0; n <= 20; n += 1.5)
    {
        double expected = 0.5 - 0.3 * cos(2 * M_PI * n / 20) + 0.2 * cos(4 * M_PI * n / 20);
        CHECK(dsp::window::cosine(n, 20, coefs, 3) == Approx(expected).margin(1e-12));
    }

    // Single coefficient is a constant
    const double one[] = {0.75};
    CHECK(dsp::window::cosine(3.3, 10, one, 1) == Approx(0.75));
    // No coefficients gives 0
    CHECK(dsp::window::cosine(3.3, 10, one, 0) == 0.0);
}

TEST_CASE("window::nuttall zero at the edges, one at the center", "[common][dsp][window]")
{
    // Nuttall coefficients sum to 1 and alternate to 0
    CHECK(dsp::window::nuttall(0, 100) == Approx(0.0).margin(1e-9));
    CHECK(dsp::window::nuttall(100, 100) == Approx(0.0).margin(1e-9));
    CHECK(dsp::window::nuttall(50, 100) == Approx(1.0).margin(1e-9));

    // Symmetric and increasing towards the center
    for (int n = 0; n <= 50; n++)
        CHECK(dsp::window::nuttall(n, 100) == Approx(dsp::window::nuttall(100 - n, 100)).margin(1e-12));
    for (int n = 1; n <= 50; n++)
        CHECK(dsp::window::nuttall(n, 100) > dsp::window::nuttall(n - 1, 100));

    // Periodic in N, which windowed_sinc relies on
    CHECK(dsp::window::nuttall(13 - 100, 100) == Approx(dsp::window::nuttall(13, 100)).margin(1e-12));
}

TEST_CASE("sinc function", "[common][dsp][window]")
{
    CHECK(dsp::sinc(0.0) == 1.0);
    CHECK(dsp::sinc(M_PI) == Approx(0.0).margin(1e-15));
    CHECK(dsp::sinc(2 * M_PI) == Approx(0.0).margin(1e-15));
    CHECK(dsp::sinc(M_PI / 2) == Approx(2.0 / M_PI));
    CHECK(dsp::sinc(-1.3) == Approx(dsp::sinc(1.3)));
}

TEST_CASE("windowed_sinc produces a symmetric unity-gain low-pass", "[common][dsp][window]")
{
    // Same parameters as the clock recovery interpolator banks: omega = hz_to_rad(0.5 / nfilt, 1)
    const int nfilt = 128, ntaps = 8;
    const double omega = M_PI / nfilt;
    auto taps = dsp::windowed_sinc(nfilt * ntaps, omega, dsp::window::nuttall, nfilt);
    REQUIRE(taps.size() == (size_t)(nfilt * ntaps));

    for (size_t i = 0; i < taps.size() / 2; i++)
        REQUIRE(taps[i] == Approx(taps[taps.size() - 1 - i]).margin(1e-6));

    // Peak is in the middle, edges are tapered to ~0 by the window
    auto peak = std::max_element(taps.begin(), taps.end()) - taps.begin();
    CHECK((peak == nfilt * ntaps / 2 || peak == nfilt * ntaps / 2 - 1));
    CHECK(fabs(taps.front()) < 1e-4 * taps[peak]);

    // DC gain is norm (here nfilt, so every polyphase arm has ~unity gain)
    double sum = std::accumulate(taps.begin(), taps.end(), 0.0);
    CHECK(sum == Approx(nfilt).epsilon(0.01));

    // Cutoff at omega: passband well below, stopband well above
    CHECK(dsp_test::magResponse(taps, omega * 0.25) / sum == Approx(1.0).margin(0.01));
    CHECK(dsp_test::toDb(dsp_test::magResponse(taps, omega * 2) / sum) < -60.0);
}

TEST_CASE("windowed_sinc norm scales the taps linearly", "[common][dsp][window]")
{
    auto a = dsp::windowed_sinc(64, M_PI / 4, dsp::window::nuttall, 1.0);
    auto b = dsp::windowed_sinc(64, M_PI / 4, dsp::window::nuttall, 3.0);
    REQUIRE(a.size() == b.size());
    for (size_t i = 0; i < a.size(); i++)
        CHECK(b[i] == Approx(3.0 * a[i]).margin(1e-7));

    // A custom window is used as-is: a constant window gives the plain truncated sinc
    auto rect = dsp::windowed_sinc(64, M_PI / 4, [](double, double)
                                   { return 1.0; });
    for (int i = 0; i < 64; i++)
    {
        double t = i - 32 + 0.5;
        CHECK(rect[i] == Approx(dsp::sinc(t * M_PI / 4) * 0.25).margin(1e-7));
    }
}
