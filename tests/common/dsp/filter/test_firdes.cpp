#include "common/dsp/dsp_test_helpers.h"
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
using dsp_test::magResponse;
using dsp_test::toDb;

namespace
{
    double sum(const std::vector<float> &v) { return std::accumulate(v.begin(), v.end(), 0.0); }

    double hzToW(double hz, double fs) { return 2 * M_PI * hz / fs; }

    void checkSymmetric(const std::vector<float> &taps, float margin = 1e-7f)
    {
        for (size_t i = 0; i < taps.size() / 2; i++)
            REQUIRE(taps[i] == Approx(taps[taps.size() - 1 - i]).margin(margin));
    }
} // namespace

TEST_CASE("firdes::convolve computes the full linear convolution", "[common][dsp][firdes]")
{
    auto r = dsp::firdes::convolve({1, 2, 3}, {0, 1, 0.5});
    REQUIRE(r.size() == 5);
    CHECK(r[0] == Approx(0.0f));
    CHECK(r[1] == Approx(1.0f));
    CHECK(r[2] == Approx(2.5f));
    CHECK(r[3] == Approx(4.0f));
    CHECK(r[4] == Approx(1.5f));

    // Identity and commutativity
    std::vector<float> a = {0.5f, -1.0f, 2.0f, 4.0f};
    CHECK(dsp::firdes::convolve(a, {1}) == a);
    auto ab = dsp::firdes::convolve(a, {3, -2});
    auto ba = dsp::firdes::convolve({3, -2}, a);
    REQUIRE(ab.size() == ba.size());
    for (size_t i = 0; i < ab.size(); i++)
        CHECK(ab[i] == Approx(ba[i]));

    // Sum of a convolution is the product of the sums
    CHECK(sum(ab) == Approx(sum(a) * 1.0));
}

TEST_CASE("firdes::low_pass tap count, symmetry and DC gain", "[common][dsp][firdes]")
{
    const double fs = 48000, fc = 6000, tw = 2000;
    auto taps = dsp::firdes::low_pass(1.0, fs, fc, tw);

    // ntaps = A * fs / (22 * tw), made odd. Hamming A = 53
    int expected = (int)(53 * fs / (22.0 * tw));
    if (expected % 2 == 0)
        expected++;
    CHECK((int)taps.size() == expected);
    CHECK(taps.size() % 2 == 1);
    checkSymmetric(taps);

    CHECK(sum(taps) == Approx(1.0).epsilon(1e-5));
    CHECK(sum(dsp::firdes::low_pass(2.5, fs, fc, tw)) == Approx(2.5).epsilon(1e-5));

    // Narrower transition = more taps
    CHECK(dsp::firdes::low_pass(1.0, fs, fc, tw / 2).size() > taps.size());
}

TEST_CASE("firdes::low_pass frequency response", "[common][dsp][firdes]")
{
    auto type = GENERATE(window::WIN_HAMMING, window::WIN_BLACKMAN, window::WIN_KAISER, window::WIN_BLACKMAN_HARRIS);
    INFO("window = " << (int)type);
    const double fs = 100000, fc = 10000, tw = 4000;
    auto taps = dsp::firdes::low_pass(1.0, fs, fc, tw, type, 7.0);

    // Passband flat, about -6 dB at the cutoff (window method), stopband attenuated
    for (double f = 0; f <= fc - tw; f += 500)
        REQUIRE(magResponse(taps, hzToW(f, fs)) == Approx(1.0).margin(0.01));
    CHECK(magResponse(taps, hzToW(fc, fs)) == Approx(0.5).margin(0.05));
    for (double f = fc + tw; f <= fs / 2; f += 500)
        REQUIRE(toDb(magResponse(taps, hzToW(f, fs))) < -40.0);
}

TEST_CASE("firdes::high_pass frequency response", "[common][dsp][firdes]")
{
    const double fs = 48000, fc = 12000, tw = 3000;
    auto taps = dsp::firdes::high_pass(1.0, fs, fc, tw);
    REQUIRE(taps.size() % 2 == 1);
    checkSymmetric(taps);

    // Normalized for unity gain at Nyquist
    CHECK(magResponse(taps, M_PI) == Approx(1.0).epsilon(1e-4));
    CHECK(toDb(magResponse(taps, 0)) < -40.0);
    CHECK(magResponse(taps, hzToW(fc, fs)) == Approx(0.5).margin(0.05));
    for (double f = 0; f <= fc - tw; f += 500)
        REQUIRE(toDb(magResponse(taps, hzToW(f, fs))) < -40.0);
    for (double f = fc + tw; f <= fs / 2; f += 500)
        REQUIRE(magResponse(taps, hzToW(f, fs)) == Approx(1.0).margin(0.01));
}

TEST_CASE("firdes::band_pass frequency response", "[common][dsp][firdes]")
{
    const double fs = 48000, f1 = 8000, f2 = 16000, tw = 2000;
    auto taps = dsp::firdes::band_pass(1.0, fs, f1, f2, tw);
    REQUIRE(taps.size() % 2 == 1);
    checkSymmetric(taps);

    // Normalized for unity gain at the band center
    CHECK(magResponse(taps, hzToW((f1 + f2) / 2, fs)) == Approx(1.0).epsilon(1e-4));
    for (double f = f1 + tw; f <= f2 - tw; f += 250)
        REQUIRE(magResponse(taps, hzToW(f, fs)) == Approx(1.0).margin(0.01));
    for (double f = 0; f <= f1 - tw; f += 250)
        REQUIRE(toDb(magResponse(taps, hzToW(f, fs))) < -40.0);
    for (double f = f2 + tw; f <= fs / 2; f += 250)
        REQUIRE(toDb(magResponse(taps, hzToW(f, fs))) < -40.0);
}

TEST_CASE("firdes::complex_band_pass passes only positive band", "[common][dsp][firdes]")
{
    const double fs = 48000, f1 = 4000, f2 = 10000, tw = 2000;
    auto taps = dsp::firdes::complex_band_pass(1.0, fs, f1, f2, tw);
    REQUIRE(taps.size() % 2 == 1);

    double center = (f1 + f2) / 2;
    CHECK(magResponse(taps, hzToW(center, fs)) == Approx(1.0).margin(0.01));
    // Mirrored negative frequency is rejected, unlike a real band-pass
    CHECK(toDb(magResponse(taps, hzToW(-center, fs))) < -40.0);
    CHECK(toDb(magResponse(taps, 0)) < -40.0);
    for (double f = f1 + tw; f <= f2 - tw; f += 250)
        REQUIRE(magResponse(taps, hzToW(f, fs)) == Approx(1.0).margin(0.01));

    // Band edges symmetric around the center
    CHECK(magResponse(taps, hzToW(f1, fs)) == Approx(magResponse(taps, hzToW(f2, fs))).margin(0.01));

    // Negative frequency band works too
    auto neg = dsp::firdes::complex_band_pass(1.0, fs, -f2, -f1, tw);
    CHECK(magResponse(neg, hzToW(-center, fs)) == Approx(1.0).margin(0.01));
    CHECK(toDb(magResponse(neg, hzToW(center, fs))) < -40.0);
}

TEST_CASE("firdes::complex_band_pass rejects invalid parameters", "[common][dsp][firdes]")
{
    CHECK_THROWS_AS(dsp::firdes::complex_band_pass(1, 0, 100, 200, 10), std::out_of_range);
    CHECK_THROWS_AS(dsp::firdes::complex_band_pass(1, 1000, 300, 200, 10), std::out_of_range);
    CHECK_THROWS_AS(dsp::firdes::complex_band_pass(1, 1000, -600, 200, 10), std::out_of_range);
    CHECK_THROWS_AS(dsp::firdes::complex_band_pass(1, 1000, 100, 600, 10), std::out_of_range);
    CHECK_THROWS_AS(dsp::firdes::complex_band_pass(1, 1000, 100, 200, 0), std::out_of_range);
    CHECK_NOTHROW(dsp::firdes::complex_band_pass(1, 1000, -500, 500, 50));
}

TEST_CASE("firdes::root_raised_cosine taps", "[common][dsp][firdes]")
{
    // Even tap counts are bumped to odd
    CHECK(dsp::firdes::root_raised_cosine(1.0, 4, 1, 0.35, 30).size() == 31);
    auto taps = dsp::firdes::root_raised_cosine(1.0, 4, 1, 0.35, 61);
    REQUIRE(taps.size() == 61);
    checkSymmetric(taps, 1e-6f);
    CHECK(sum(taps) == Approx(1.0).epsilon(1e-5));
    CHECK(sum(dsp::firdes::root_raised_cosine(3.0, 4, 1, 0.35, 61)) == Approx(3.0).epsilon(1e-5));

    // Peak in the middle
    auto peak = std::max_element(taps.begin(), taps.end());
    CHECK(peak - taps.begin() == 30);

    // Bandwidth is (1 + alpha) * Rs / 2: DC passes, beyond that it's rejected
    double edge = hzToW(0.5 * (1 + 0.35), 4);
    CHECK(toDb(magResponse(taps, 0) / magResponse(taps, 0)) == Approx(0.0));
    CHECK(toDb(magResponse(taps, edge + 0.15) / magResponse(taps, 0)) < -25.0);
    // Half amplitude squared (-3 dB) at the symbol rate / 2
    CHECK(magResponse(taps, hzToW(0.5, 4)) / magResponse(taps, 0) == Approx(sqrt(0.5)).margin(0.03));
}

TEST_CASE("firdes::root_raised_cosine squared is Nyquist (no ISI)", "[common][dsp][firdes]")
{
    const int sps = 4;
    auto taps = dsp::firdes::root_raised_cosine(1.0, sps, 1, 0.5, 8 * sps * 2 + 1);
    auto rc = dsp::firdes::convolve(taps, taps);
    int center = rc.size() / 2;

    // Raised cosine has zero crossings at every non-zero multiple of the symbol period
    for (int k = 1; k <= 6; k++)
    {
        INFO("k = " << k);
        CHECK(fabs(rc[center + k * sps]) < 0.02 * rc[center]);
        CHECK(fabs(rc[center - k * sps]) < 0.02 * rc[center]);
    }
}

TEST_CASE("firdes::root_raised_cosine with alpha = 1 takes the special case path", "[common][dsp][firdes]")
{
    // With alpha = 1 and spb = 4, x3 == 0 at xindx = +-1 and those taps are forced to -1 before normalization
    auto taps = dsp::firdes::root_raised_cosine(1.0, 4, 1, 1.0, 21);
    REQUIRE(taps.size() == 21);
    CHECK(sum(taps) == Approx(1.0).epsilon(1e-5));
    checkSymmetric(taps, 1e-6f);
    for (float t : taps)
        CHECK(std::isfinite(t));
}

TEST_CASE("firdes::gaussian taps", "[common][dsp][firdes]")
{
    auto taps = dsp::firdes::gaussian(1.0, 4, 0.5, 31);
    REQUIRE(taps.size() == 31);
    CHECK(sum(taps) == Approx(1.0).epsilon(1e-5));
    for (float t : taps)
        CHECK(t >= 0.0f); // Far tails underflow to 0 in float

    // Same as GNU Radio: sample times are i - ntaps/2 + 1, so for odd ntaps the pulse
    // is centered between taps 14 and 15 (t = -0.5 / +0.5), and the last tap has no mirror
    CHECK(taps[14] == Approx(taps[15]).epsilon(1e-6));
    for (int i = 0; i < 15; i++)
        CHECK(taps[i] == Approx(taps[29 - i]).epsilon(1e-5));
    CHECK(std::max_element(taps.begin(), taps.end()) - taps.begin() >= 14);
    CHECK(std::max_element(taps.begin(), taps.end()) - taps.begin() <= 15);

    // Lower BT = wider pulse = smaller peak
    auto wide = dsp::firdes::gaussian(1.0, 4, 0.3, 31);
    CHECK(wide[15] < taps[15]);
    CHECK(sum(dsp::firdes::gaussian(2.0, 4, 0.5, 31)) == Approx(2.0).epsilon(1e-5));

    // Matches exp(-0.5 (s t / spb)^2), s = 2 pi BT / sqrt(ln 2), with t = 3.5 vs t = 0.5
    double s = 2 * M_PI * 0.5 / sqrt(log(2.0));
    double ratio = exp(-0.5 * pow(s / 4.0, 2) * (3.5 * 3.5 - 0.5 * 0.5));
    CHECK(taps[15 + 3] / taps[15] == Approx(ratio).epsilon(1e-4));
}

TEST_CASE("firdes::hilbert taps", "[common][dsp][firdes]")
{
    CHECK_THROWS_AS(dsp::firdes::hilbert(32, window::WIN_HAMMING, 0), std::out_of_range);

    auto taps = dsp::firdes::hilbert(31, window::WIN_HAMMING, 0);
    REQUIRE(taps.size() == 31);
    const int h = 15;

    // Anti-symmetric, zero at center and at even offsets
    CHECK(taps[h] == 0.0f);
    for (int i = 1; i <= h; i++)
    {
        CHECK(taps[h + i] == Approx(-taps[h - i]).margin(1e-7));
        if (i % 2 == 0)
            CHECK(taps[h + i] == 0.0f);
        else
            CHECK(taps[h + i] > 0.0f);
    }

    // Normalized to unity gain at fs/4, DC rejected
    CHECK(magResponse(taps, M_PI / 2) == Approx(1.0).epsilon(1e-5));
    CHECK(magResponse(taps, 0) == Approx(0.0).margin(1e-6));
    // Band-pass-ish all-pass in the middle of the band
    CHECK(magResponse(taps, M_PI / 4) == Approx(1.0).margin(0.05));
}

TEST_CASE("firdes::design_resampler_filter_float", "[common][dsp][firdes]")
{
    // Interpolating: gain = interpolation, cutoff around 1 / (2 * interp) of the upsampled rate
    auto up = dsp::firdes::design_resampler_filter_float(3, 2, 0.4);
    REQUIRE(up.size() % 2 == 1);
    checkSymmetric(up, 1e-6f);
    CHECK(sum(up) == Approx(3.0).epsilon(1e-4));
    // At the upsampled rate (interp = Fs), the passband 0.4 of an input sample rate passes
    CHECK(magResponse(up, 2 * M_PI * 0.3 / 3) == Approx(3.0).margin(0.05));
    CHECK(toDb(magResponse(up, 2 * M_PI * 0.6 / 3) / 3.0) < -60.0);

    // Decimating: the cutoff scales with the rate
    auto down = dsp::firdes::design_resampler_filter_float(1, 4, 0.4);
    CHECK(sum(down) == Approx(1.0).epsilon(1e-4));
    CHECK(magResponse(down, 2 * M_PI * 0.3 / 4) == Approx(1.0).margin(0.02));
    CHECK(toDb(magResponse(down, 2 * M_PI * 0.6 / 4)) < -60.0);
}
