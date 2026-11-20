#include "dsp/channel_model/channel_model_simple.h"
#include "dsp/ndsp_sig_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using Catch::Approx;
using namespace satdump::ndsp;

namespace
{
    // Note : the noise generator is seeded from the clock (dsp::Random seed 0), so the noise
    // checks are statistical, with tolerances way above the estimation error for these lengths.
    std::vector<complex_t> runModel(const std::vector<complex_t> &in, double signal_level, double noise_level, double freq_shift, double samplerate = 1e6)
    {
        auto blk = std::make_shared<ChannelModelSimpleBlock>();
        REQUIRE(blk->set_cfg("samplerate", samplerate) == Block::RES_OK);
        REQUIRE(blk->set_cfg("signal_level", signal_level) == Block::RES_OK);
        REQUIRE(blk->set_cfg("noise_level", noise_level) == Block::RES_OK);
        REQUIRE(blk->set_cfg("freq_shift", freq_shift) == Block::RES_OK);
        auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(blk, in);
        REQUIRE(r.completed);
        REQUIRE(r.output().size() == in.size());
        return r.output();
    }
} // namespace

TEST_CASE("ndsp ChannelModelSimpleBlock applies the frequency shift", "[dsp][channel_model]")
{
    const double fs = 1e6;
    std::vector<complex_t> in(50000, complex_t(1, 0));
    for (double shift : {10e3, -123e3})
    {
        INFO("shift " << shift);
        // signal_level -26.5 gives unity gain (see below), negligible noise
        auto out = runModel(in, -26.5, -300, shift, fs);
        CHECK(ndsp_sig_test::estimateFreq(out, 1000, out.size()) == Approx(2 * M_PI * shift / fs).epsilon(1e-4));
        CHECK(ndsp_sig_test::toneAmplitude(out, 2 * M_PI * shift / fs, 1000, out.size()) == Approx(1.0).epsilon(1e-3));
    }
}

TEST_CASE("ndsp ChannelModelSimpleBlock frequency shift ramps up smoothly", "[dsp][channel_model]")
{
    // Instantaneous frequency follows a first order low-pass toward the target, alpha = 0.1
    std::vector<complex_t> in(200, complex_t(1, 0));
    const double fs = 1e6, shift = 50e3;
    auto out = runModel(in, -26.5, -300, shift, fs);
    double target = 2 * M_PI * shift / fs;
    double f = 0;
    for (size_t i = 1; i < 60; i++)
    {
        // out[i] / out[i - 1] rotates by the frequency used at step i - 1
        double inst = std::arg(ndsp_sig_test::toStd(out[i]) * std::conj(ndsp_sig_test::toStd(out[i - 1])));
        INFO("sample " << i);
        CHECK(inst == Approx(f).margin(1e-4));
        f = f * 0.9 + target * 0.1;
    }
}

TEST_CASE("ndsp ChannelModelSimpleBlock signal and noise levels", "[dsp][channel_model]")
{
    // As implemented : a unit input comes out with a power of 10^((signal_level + 26.5) / 10),
    // the noise has a power of 10^((noise_level + 30) / 10). So the SNR in dB is
    // signal_level - noise_level - 3.5.
    const size_t n = 200000;
    std::vector<complex_t> ones(n, complex_t(1, 0));
    std::vector<complex_t> zeros(n, complex_t(0, 0));

    SECTION("signal power")
    {
        for (double s : {-26.5, -46.5, -16.5})
        {
            INFO("signal level " << s);
            auto out = runModel(ones, s, -300, 0);
            CHECK(ndsp_sig_test::meanPower(out, 0, n) == Approx(std::pow(10, (s + 26.5) / 10)).epsilon(1e-4));
        }
    }

    SECTION("noise power and statistics")
    {
        for (double nl : {-30.0, -40.0, -20.0})
        {
            INFO("noise level " << nl);
            auto out = runModel(zeros, -100, nl, 0);
            double expected = std::pow(10, (nl + 30) / 10);
            CHECK(ndsp_sig_test::meanPower(out, 0, n) == Approx(expected).epsilon(0.03));

            // Zero-mean, circular, gaussian
            double mr = 0, mi = 0, vr = 0, vi = 0, cross = 0, k4 = 0;
            for (auto &v : out)
            {
                mr += v.real, mi += v.imag;
                vr += v.real * v.real, vi += v.imag * v.imag, cross += v.real * v.imag;
                k4 += std::pow(v.real, 4);
            }
            mr /= n, mi /= n, vr /= n, vi /= n, cross /= n, k4 /= n;
            CHECK(std::fabs(mr) < 0.02 * std::sqrt(expected));
            CHECK(std::fabs(mi) < 0.02 * std::sqrt(expected));
            CHECK(vr == Approx(expected / 2).epsilon(0.03));
            CHECK(vi == Approx(expected / 2).epsilon(0.03));
            CHECK(std::fabs(cross) < 0.02 * expected);
            // Kurtosis of a gaussian is 3
            CHECK(k4 / (vr * vr) == Approx(3.0).epsilon(0.05));
        }
    }

    SECTION("SNR")
    {
        auto out = runModel(ones, -100, -120, 0);
        double sig = std::pow(10, (-100 + 26.5) / 10);
        // Noise power around the expected signal. Taking total - sig instead leaves the signal x noise
        // cross term, about 0.09 dB rms here, which the clock-seeded noise exceeded now and then
        double noise = 0;
        for (auto &v : out)
            noise += std::norm(ndsp_sig_test::toStd(v) - std::sqrt(sig));
        noise /= n;
        double snr_db = 10 * log10(sig / noise);
        CHECK(snr_db == Approx(-100 - -120 - 3.5).margin(0.2));
    }
}

TEST_CASE("ndsp ChannelModelSimpleBlock configuration getters", "[dsp][channel_model]")
{
    ChannelModelSimpleBlock blk;
    CHECK(blk.d_id == "channel_model_simple_cc");
    CHECK(blk.get_cfg("samplerate") == 1e6);
    CHECK(blk.get_cfg("noise_level") == -150);
    CHECK(blk.get_cfg("signal_level") == -120);
    CHECK(blk.get_cfg("freq_shift") == 0);
    CHECK(blk.get_cfg("alpha").get<float>() == Approx(0.1));

    REQUIRE(blk.set_cfg("samplerate", 2e6) == Block::RES_OK);
    REQUIRE(blk.set_cfg("noise_level", -140) == Block::RES_OK);
    REQUIRE(blk.set_cfg("signal_level", -110) == Block::RES_OK);
    REQUIRE(blk.set_cfg("freq_shift", 1234.5) == Block::RES_OK);
    REQUIRE(blk.set_cfg("alpha", 0.25) == Block::RES_OK);
    CHECK(blk.get_cfg("samplerate") == 2e6);
    CHECK(blk.get_cfg("noise_level") == -140);
    CHECK(blk.get_cfg("signal_level") == -110);
    CHECK(blk.get_cfg("freq_shift") == 1234.5);
    CHECK(blk.get_cfg("alpha").get<float>() == Approx(0.25));
    CHECK_THROWS(blk.set_cfg("x", 1));
}

// Bug: channel_model_simple.h:43 declares the parameter as "d_alpha" in get_cfg_list(),
// while get_cfg() / set_cfg() only know "alpha", so getting all the parameters throws.
TEST_CASE("ndsp ChannelModelSimpleBlock get_cfg returns every listed parameter", "[dsp][channel_model][!shouldfail]")
{
    ChannelModelSimpleBlock blk;
    nlohmann::json all;
    REQUIRE_NOTHROW(all = ndsp_sig_test::getAll(blk));
    CHECK(all.size() == blk.get_cfg_list().size());
}
