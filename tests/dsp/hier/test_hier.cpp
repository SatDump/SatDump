#include "common/dsp/filter/firdes.h"
#include "dsp/hier/audio_demod.h"
#include "dsp/hier/gfsk_mod.h"
#include "dsp/hier/psk_demod.h"
#include "dsp/hier/psk_demod_fast.h"
#include "dsp/hier/psk_demod_superfast.h"
#include "../filter/ndsp_sig_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;
using namespace satdump::ndsp;

namespace
{
    const double kSr = 4e5, kSymRate = 1e5; // 4 samples per symbol
    const int kRrcTaps = 61;
    // Carrier offset, inside the pull-in range of the default BPSK Costas loop at the symbol rate
    const double kOffsetHz = 50;

    // RRC-shaped PSK with a carrier offset and white noise at a given Es/N0 (after matched filtering)
    std::vector<complex_t> pskSignal(const std::vector<complex_t> &syms, double esn0_db)
    {
        const int sps = 4;
        std::vector<complex_t> up(syms.size() * sps, complex_t(0, 0));
        for (size_t i = 0; i < syms.size(); i++)
            up[i * sps] = syms[i];
        auto rrc = dsp::firdes::root_raised_cosine(1.0, kSr, kSymRate, 0.35, kRrcTaps);
        auto s = ndsp_sig_test::filterSame(up, rrc);
        s = ndsp_sig_test::rotate(s, 2 * ndsp_sig_test::kPi * kOffsetHz / kSr, 0.5);

        // Same filter on the receive side : symbol SNR = sum(g^2) / (2 sigma^2)
        double eg = 0;
        for (float t : rrc)
            eg += (double)t * t;
        double sigma = std::sqrt(eg / (2.0 * std::pow(10, esn0_db / 10)));
        ndsp_sig_test::addNoise(s, sigma, 77);
        return s;
    }

    template <typename B>
    void checkPskDemod(const std::string &constellation, bool check_pll_freq)
    {
        INFO("constellation " << constellation);
        int order = constellation == "bpsk" ? 2 : 4;
        auto syms = order == 2 ? ndsp_sig_test::bpskSymbols(16000, 31) : ndsp_sig_test::qpskSymbols(16000, 32);
        auto in = pskSignal(syms, 15);

        auto demod = std::make_shared<B>();
        REQUIRE(demod->set_cfg("constellation", constellation) == Block::RES_OK);
        REQUIRE(demod->set_cfg("samplerate", kSr) == Block::RES_OK);
        REQUIRE(demod->set_cfg("symbolrate", kSymRate) == Block::RES_OK);
        REQUIRE(demod->set_cfg("rrc_ntaps", kRrcTaps) == Block::RES_OK);
        auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(demod, in, ndsp_sig_test::chunked(8192));
        REQUIRE(r.completed);
        auto &out = r.output();

        // About one sample per symbol
        CHECK((double)out.size() == Approx((double)syms.size()).epsilon(0.02));

        // Symbols recovered after convergence
        auto m = ndsp_sig_test::matchSymbols(syms, out, order, 8000, 60);
        INFO("lag " << m.lag);
        CHECK(m.ratio > 0.999);

        // Output normalized by the AGC (0.6 reference). The AGC runs on every sample
        // before the clock recovery, so the level at the symbol instants is only about that
        double mag = 0;
        for (size_t i = out.size() - 8000; i < out.size(); i++)
            mag += std::abs(ndsp_sig_test::toStd(out[i]));
        CHECK(mag / 8000 == Approx(0.6).margin(0.2));

        // M2M4 SNR estimate close to the actual 15 dB
        CHECK(demod->get_cfg("snr").template get<double>() == Approx(15).margin(2.5));

        // Carrier offset, in Hz
        if (check_pll_freq)
            CHECK(demod->get_cfg("pll_freq").template get<double>() == Approx(kOffsetHz).margin(10));
    }

    template <typename B>
    void checkPskConfig()
    {
        B d;
        auto list = d.get_cfg_list();
        for (auto k : {"constellation", "samplerate", "symbolrate", "advanced", "pll_freq", "snr"})
            CHECK(list.contains(k));
        CHECK(!list.contains("rrc_alpha"));

        CHECK(d.get_cfg("constellation") == "bpsk");
        CHECK(d.get_cfg("samplerate") == 6e6);
        CHECK(d.get_cfg("symbolrate") == 2e6);
        CHECK(d.get_cfg("advanced") == false);
        CHECK(d.set_cfg("constellation", "qpsk") == Block::RES_OK);
        CHECK(d.get_cfg("constellation") == "qpsk");
        CHECK(d.get_cfg("pll_order") == 4);
        CHECK(d.set_cfg("constellation", "8psk") == Block::RES_ERR);
        CHECK(d.get_cfg("constellation") == "qpsk");

        // Sample / symbol rates go down to the RRC and clock recovery
        CHECK(d.set_cfg("samplerate", 1e6) == Block::RES_OK);
        CHECK(d.set_cfg("symbolrate", 2.5e5) == Block::RES_OK);
        CHECK(d.get_cfg("rrc_samplerate") == 1e6);
        CHECK(d.get_cfg("rrc_symbolrate") == 2.5e5);
        CHECK(d.get_cfg("rec_omega").template get<double>() == Approx(4.0));
        CHECK(d.get_cfg("agc_reference").template get<double>() == Approx(0.6));

        // Advanced mode exposes the sub-blocks parameters
        CHECK(d.set_cfg("advanced", true) == Block::RES_LISTUPD);
        list = d.get_cfg_list();
        for (auto k : {"rrc_alpha", "rrc_ntaps", "agc_rate", "rec_omegaGain", "pll_loop_bw"})
            CHECK(list.contains(k));
        CHECK(list["rrc_alpha"]["name"] == "RRC Alpha");
        CHECK(d.set_cfg("rrc_alpha", 0.5) == Block::RES_OK);
        CHECK(d.get_cfg("rrc_alpha").template get<double>() == 0.5);
        CHECK(d.set_cfg("pll_loop_bw", 0.01) == Block::RES_OK);
        CHECK(d.get_cfg("pll_loop_bw").template get<double>() == Approx(0.01));

        CHECK(d.set_cfg("unknown", 1) == Block::RES_ERR);
        CHECK(d.get_cfg("unknown").is_null());
    }
} // namespace

TEST_CASE("ndsp PSKDemodHierBlock demodulates noisy BPSK and QPSK", "[dsp][hier]")
{
    // Costas loop before clock recovery here, the pll_freq status is checked below
    checkPskDemod<PSKDemodHierBlock>("bpsk", false);
    checkPskDemod<PSKDemodHierBlock>("qpsk", false);
}

// Bug : psk_demod.h:152 converts the Costas frequency to Hz with the symbol rate, but in
// PSKDemodHierBlock the Costas loop runs before the clock recovery, at the sample rate, so the
// reported pll_freq is off by a factor samplerate / symbolrate (12.5 Hz instead of 50 Hz here).
TEST_CASE("ndsp PSKDemodHierBlock reports the carrier offset in Hz", "[dsp][hier][!shouldfail]")
{
    auto syms = ndsp_sig_test::bpskSymbols(16000, 33);
    auto in = pskSignal(syms, 20);
    auto demod = std::make_shared<PSKDemodHierBlock>();
    REQUIRE(demod->set_cfg("samplerate", kSr) == Block::RES_OK);
    REQUIRE(demod->set_cfg("symbolrate", kSymRate) == Block::RES_OK);
    auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(demod, in, ndsp_sig_test::chunked(8192));
    REQUIRE(r.completed);
    CHECK(demod->get_cfg("pll_freq").get<double>() == Approx(kOffsetHz).margin(10));
}

TEST_CASE("ndsp PSKDemodFastHierBlock demodulates noisy BPSK and QPSK", "[dsp][hier]")
{
    checkPskDemod<PSKDemodFastHierBlock>("bpsk", true);
    checkPskDemod<PSKDemodFastHierBlock>("qpsk", true);
}

TEST_CASE("ndsp PSKDemodSuperFastHierBlock demodulates noisy BPSK and QPSK", "[dsp][hier]")
{
    checkPskDemod<PSKDemodSuperFastHierBlock>("bpsk", true);
    checkPskDemod<PSKDemodSuperFastHierBlock>("qpsk", true);
}

TEST_CASE("ndsp PSKDemod hier blocks configuration", "[dsp][hier]")
{
    checkPskConfig<PSKDemodHierBlock>();
    checkPskConfig<PSKDemodFastHierBlock>();
    checkPskConfig<PSKDemodSuperFastHierBlock>();
    CHECK(PSKDemodHierBlock().d_id == "psk_demod_cc");
    CHECK(PSKDemodFastHierBlock().d_id == "psk_demod_fast_cc");
    CHECK(PSKDemodSuperFastHierBlock().d_id == "psk_demod_superfast_cc");
}

TEST_CASE("ndsp GFSKModHierBlock outputs a constant envelope at +-sensitivity", "[dsp][hier]")
{
    // Runs of +1 and -1, the frequency settles to +-k rad/sample
    std::vector<float> in;
    for (int run = 0; run < 6; run++)
        for (int i = 0; i < 400; i++)
            in.push_back(run % 2 ? -1.0f : 1.0f);

    auto mod = std::make_shared<GFSKModHierBlock>();
    REQUIRE(mod->set_cfg("sensitivity", 0.5) == Block::RES_OK);
    auto r = ndsp_sig_test::runBlock<float, complex_t>(mod, in, ndsp_sig_test::chunked(500));
    REQUIRE(r.completed);
    auto &out = r.output();
    // Interpolation by 2
    CHECK((double)out.size() == Approx(2.0 * in.size()).margin(100));

    for (auto &v : out)
        REQUIRE(std::abs(ndsp_sig_test::toStd(v)) == Approx(1.0).margin(1e-4));

    // Middle of the runs : instantaneous frequency is +-k
    std::vector<double> inst(out.size(), 0.0);
    for (size_t i = 1; i < out.size(); i++)
        inst[i] = std::arg(ndsp_sig_test::toStd(out[i]) * std::conj(ndsp_sig_test::toStd(out[i - 1])));
    for (int run = 0; run < 5; run++)
    {
        INFO("run " << run);
        size_t center = run * 800 + 400;
        double sign = run % 2 ? -1.0 : 1.0;
        for (size_t i = center - 200; i < center + 200; i++)
            REQUIRE(inst[i] == Approx(sign * 0.5).margin(1e-3));
    }
    // Never deviates more than k
    for (double f : inst)
        REQUIRE(std::fabs(f) <= 0.5 + 1e-3);
}

TEST_CASE("ndsp GFSKModHierBlock configuration", "[dsp][hier]")
{
    GFSKModHierBlock mod;
    CHECK(mod.d_id == "gfsk_mod_cc");
    auto def = ndsp_sig_test::getAll(mod);
    CHECK(def["sensitivity"].get<float>() == Approx(M_PI / 4));
    CHECK(def["alpha"].get<float>() == Approx(0.35));
    CHECK(def["ntaps"] == 31);
    CHECK(def["interpolation"] == 2);
    CHECK(ndsp_sig_test::setAll(mod, {{"sensitivity", 0.25}, {"alpha", 0.5}, {"ntaps", 21}, {"interpolation", 4}}) == Block::RES_OK);
    auto got = ndsp_sig_test::getAll(mod);
    CHECK(got["sensitivity"].get<float>() == Approx(0.25));
    CHECK(got["alpha"].get<float>() == Approx(0.5));
    CHECK(got["ntaps"] == 21);
    CHECK(got["interpolation"] == 4);
    CHECK(mod.set_cfg("nope", 1) == Block::RES_ERR);
    CHECK(mod.get_cfg("nope").is_null());
    CHECK(mod.get_inputs()[0].type == DSP_SAMPLE_TYPE_F32);
    CHECK(mod.get_outputs()[0].type == DSP_SAMPLE_TYPE_CF32);
}

TEST_CASE("ndsp GFSKModHierBlock interpolation setting changes the output rate", "[dsp][hier]")
{
    std::vector<float> in(3000, 1.0f);
    for (int interp : {3, 4, 8})
    {
        INFO("interpolation " << interp);
        auto mod = std::make_shared<GFSKModHierBlock>();
        REQUIRE(mod->set_cfg("interpolation", interp) == Block::RES_OK);
        auto r = ndsp_sig_test::runBlock<float, complex_t>(mod, in);
        REQUIRE(r.completed);
        CHECK((double)r.output().size() == Approx((double)interp * in.size()).margin(400));
    }
}

// Bug : gfsk_mod.h:37 always convolves the gaussian taps with {1, 1}, a 2-samples
// rectangular pulse, instead of one `interpolation` samples long. Each polyphase branch
// then has a gain of 2 / interpolation, so the frequency deviation is only right for
// interpolation 2 (it is halved with 4, ...).
TEST_CASE("ndsp GFSKModHierBlock deviation does not depend on the interpolation", "[dsp][hier][!shouldfail]")
{
    std::vector<float> in(3000, 1.0f);
    auto mod = std::make_shared<GFSKModHierBlock>();
    REQUIRE(mod->set_cfg("interpolation", 4) == Block::RES_OK);
    REQUIRE(mod->set_cfg("sensitivity", 0.2) == Block::RES_OK);
    auto r = ndsp_sig_test::runBlock<float, complex_t>(mod, in);
    REQUIRE(r.completed);
    CHECK(ndsp_sig_test::estimateFreq(r.output(), 1000, r.output().size()) == Approx(0.2).epsilon(1e-3));
}

namespace
{
    const double kAudioIn = 48000, kAudioOut = 16000;

    // The selector stops at the first terminator it sees, on any of its inputs, so wait
    // for most of the output before terminating, or the tail may be dropped
    std::vector<float> runAudio(const std::string &mode, const std::vector<complex_t> &in)
    {
        auto demod = std::make_shared<AudioDemodHierBlock>();
        REQUIRE(demod->set_cfg("samplerate", kAudioIn) == Block::RES_OK);
        REQUIRE(demod->set_cfg("audio_samplerate", kAudioOut) == Block::RES_OK);
        REQUIRE(demod->set_cfg("mode", mode) == Block::RES_OK);
        ndsp_sig_test::RunOptions opt;
        opt.chunk = 4096;
        size_t expect = (size_t)(0.9 * in.size() * kAudioOut / kAudioIn);
        opt.before_terminate = [expect](const std::function<size_t(size_t)> &count) { ndsp_sig_test::waitForCount(count, 0, expect); };
        auto r = ndsp_sig_test::runBlock<complex_t, float>(demod, in, opt);
        REQUIRE(r.completed);
        REQUIRE(r.output().size() >= expect);
        CHECK(r.output().size() <= in.size() * kAudioOut / kAudioIn + 1);
        return r.output();
    }
} // namespace

TEST_CASE("ndsp AudioDemodHierBlock demodulates NFM", "[dsp][hier]")
{
    // 1 kHz tone, 3 kHz deviation
    const size_t n = 48000;
    const double dev = 3000, fa = 1000;
    std::vector<complex_t> in(n);
    double phase = 0;
    for (size_t i = 0; i < n; i++)
    {
        in[i] = ndsp_sig_test::fromStd(std::polar(0.3, phase));
        phase += 2 * ndsp_sig_test::kPi * dev / kAudioIn * std::cos(2 * ndsp_sig_test::kPi * fa * i / kAudioIn);
    }
    auto out = runAudio("nfm", in);
    double w = 2 * ndsp_sig_test::kPi * fa / kAudioOut;
    size_t s = 2000;
    CHECK(ndsp_sig_test::estimateRealFreq(out, s, out.size()) == Approx(w).epsilon(0.01));
    // Phase difference amplitude, divided by 20 : the quadrature demodulator's
    // "gain" setting is the inverse of the actual gain
    double expected = 2 * ndsp_sig_test::kPi * dev / kAudioIn / 20.0;
    CHECK(ndsp_sig_test::toneAmplitude(out, w, s, out.size()) == Approx(expected).epsilon(0.05));
}

TEST_CASE("ndsp AudioDemodHierBlock demodulates USB and LSB", "[dsp][hier]")
{
    const size_t n = 48000;
    const double fa = 1200;
    double w = 2 * ndsp_sig_test::kPi * fa / kAudioOut;
    size_t s = 3000;

    // USB : tone above the carrier, LSB : below. Output level set by the AGC (0.2 reference)
    auto usb = runAudio("usb", ndsp_sig_test::complexTone(n, 2 * ndsp_sig_test::kPi * fa / kAudioIn, 0.5));
    CHECK(ndsp_sig_test::estimateRealFreq(usb, s, usb.size()) == Approx(w).epsilon(0.01));
    CHECK(ndsp_sig_test::toneAmplitude(usb, w, s, usb.size()) == Approx(0.2).epsilon(0.1));

    auto lsb = runAudio("lsb", ndsp_sig_test::complexTone(n, -2 * ndsp_sig_test::kPi * fa / kAudioIn, 0.5));
    CHECK(ndsp_sig_test::estimateRealFreq(lsb, s, lsb.size()) == Approx(w).epsilon(0.01));
    CHECK(ndsp_sig_test::toneAmplitude(lsb, w, s, lsb.size()) == Approx(0.2).epsilon(0.1));
}

TEST_CASE("ndsp AudioDemodHierBlock demodulates AM", "[dsp][hier]")
{
    const size_t n = 48000;
    const double fa = 800;
    std::vector<complex_t> in(n);
    for (size_t i = 0; i < n; i++)
        in[i] = ndsp_sig_test::fromStd(std::polar(0.5 * (1.0 + 0.5 * std::cos(2 * ndsp_sig_test::kPi * fa * i / kAudioIn)), 0.3));
    auto out = runAudio("am", in);
    // The fast AGC flattens the envelope partially, but the tone is still the main component
    size_t s = 2000;
    double mean = 0;
    for (size_t i = s; i < out.size(); i++)
        mean += out[i];
    mean /= (double)(out.size() - s);
    std::vector<float> ac(out.begin() + s, out.end());
    for (auto &v : ac)
        v -= mean;
    double w = 2 * ndsp_sig_test::kPi * fa / kAudioOut;
    CHECK(ndsp_sig_test::estimateRealFreq(ac, 0, ac.size()) == Approx(w).epsilon(0.02));
    double a1 = ndsp_sig_test::toneAmplitude(ac, w, 0, ac.size());
    CHECK(a1 > 5 * ndsp_sig_test::toneAmplitude(ac, 2 * w, 0, ac.size()));
    CHECK(a1 > 5 * ndsp_sig_test::toneAmplitude(ac, 3 * w, 0, ac.size()));
}

TEST_CASE("ndsp AudioDemodHierBlock configuration", "[dsp][hier]")
{
    AudioDemodHierBlock d;
    CHECK(d.d_id == "audio_demod_cc");
    CHECK(ndsp_sig_test::getAll(d) == nlohmann::json({{"samplerate", 6e6}, {"audio_samplerate", 48e3}, {"mode", "nfm"}}));
    CHECK(d.get_cfg_list()["mode"]["list"].size() == 4);
    for (auto m : {"am", "usb", "lsb", "nfm"})
    {
        CHECK(d.set_cfg("mode", m) == Block::RES_OK);
        CHECK(d.get_cfg("mode") == m);
    }
    CHECK(d.set_cfg("mode", "wfm") == Block::RES_ERR);
    CHECK(d.get_cfg("mode") == "nfm");
    CHECK(d.set_cfg("samplerate", 96000) == Block::RES_OK);
    CHECK(d.set_cfg("audio_samplerate", 8000) == Block::RES_OK);
    CHECK(d.get_cfg("samplerate") == 96000);
    CHECK(d.get_cfg("audio_samplerate") == 8000);
    CHECK(d.set_cfg("other", 1) == Block::RES_ERR);
    CHECK_THROWS(d.get_cfg("other"));
}
