#include "common/dsp/utils/random.h"
#include "common/dsp/utils/snr_estimator.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

using Catch::Approx;

namespace
{
    // Unit-power QPSK plus complex AWGN at the requested Es/N0
    std::vector<complex_t> noisyQpsk(size_t n, double snr_db, unsigned seed)
    {
        dsp::Random rng(seed);
        double noise_sigma = sqrt(pow(10.0, -snr_db / 10.0) / 2.0); // Per component
        std::vector<complex_t> v(n);
        for (auto &x : v)
        {
            float i = rng.ran_int() ? M_SQRT1_2 : -M_SQRT1_2;
            float q = rng.ran_int() ? M_SQRT1_2 : -M_SQRT1_2;
            x = complex_t(i + noise_sigma * rng.gasdev(), q + noise_sigma * rng.gasdev());
        }
        return v;
    }
} // namespace

TEST_CASE("M2M4SNREstimator estimates the SNR of noisy QPSK", "[common][dsp][snr_estimator]")
{
    for (double snr : {5.0, 10.0, 15.0, 20.0})
    {
        INFO("SNR = " << snr << " dB");
        auto sig = noisyQpsk(100000, snr, 100 + (unsigned)snr);
        M2M4SNREstimator est(0.001);
        est.update(sig.data(), sig.size());
        CHECK(est.snr() == Approx(snr).margin(0.5));
        // Signal power is 1 (0 dB), noise power is -snr dB
        CHECK(est.signal() == Approx(0.0).margin(0.3));
        CHECK(est.noise() == Approx(-snr).margin(0.6));
    }
}

TEST_CASE("M2M4SNREstimator is scale invariant", "[common][dsp][snr_estimator]")
{
    auto sig = noisyQpsk(50000, 12.0, 200);
    auto scaled = sig;
    for (auto &x : scaled)
        x *= 0.01f;

    M2M4SNREstimator a, b;
    a.update(sig.data(), sig.size());
    b.update(scaled.data(), scaled.size());
    CHECK(a.snr() == Approx(b.snr()).margin(0.01));
    // Powers move by 20 log10(0.01) = -40 dB
    CHECK(b.signal() - a.signal() == Approx(-40.0).margin(0.01));
}

TEST_CASE("M2M4SNREstimator never reports a negative SNR", "[common][dsp][snr_estimator]")
{
    // Pure gaussian noise: M2^2 * 2 - M4 is ~0, signal estimate collapses
    dsp::Random rng(300);
    std::vector<complex_t> noise(50000);
    for (auto &x : noise)
        x = complex_t(rng.gasdev(), rng.gasdev());

    M2M4SNREstimator est;
    est.update(noise.data(), noise.size());
    float snr = est.snr();
    // May be NaN-free 0 or a small positive value, but never below 0
    CHECK((snr >= 0.0f || std::isnan(snr)));
    CHECK(snr < 5.0f);
}

TEST_CASE("M2M4SNREstimator tracks with an exponential average", "[common][dsp][snr_estimator]")
{
    // Fresh estimator at alpha = 0.1 converges in a few tens of samples to the moments
    M2M4SNREstimator est(0.1);
    std::vector<complex_t> c(500, complex_t(2.0f, 0.0f));
    est.update(c.data(), c.size());
    // signal() / noise() report values computed by the last snr() call
    est.snr();
    // Constant modulus, no noise: signal = |x|^2 = 4 (6.02 dB)
    CHECK(est.signal() == Approx(10 * log10(4.0)).margin(0.01));
}
