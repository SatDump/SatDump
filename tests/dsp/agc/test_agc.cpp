#include "dsp/agc/agc.h"
#include "dsp/agc/agc_bias.h"
#include "dsp/agc/agc_fast.h"
#include "../filter/ndsp_sig_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;
using namespace satdump::ndsp;

namespace
{
    template <typename B>
    void configureAgc(B &agc, float rate, float reference, float gain, float max_gain)
    {
        REQUIRE(ndsp_sig_test::setAll(agc, nlohmann::json{{"rate", rate}, {"reference", reference}, {"gain", gain}, {"max_gain", max_gain}}) == Block::RES_OK);
    }
} // namespace

TEST_CASE("ndsp AGCBlock<complex_t> converges to the reference magnitude", "[dsp][agc]")
{
    for (double amp : {0.05, 0.5, 20.0})
    {
        INFO("input amplitude = " << amp);
        // Gain time constant is 1 / (rate * amp) samples, 2000 at worst here
        auto in = ndsp_sig_test::complexTone(40000, 0.1, amp);
        auto agc = std::make_shared<AGCBlock<complex_t>>();
        configureAgc(*agc, 1e-2, 1.0f, 1.0f, 65536);
        auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(agc, in);
        REQUIRE(r.completed);
        auto &out = r.output();
        REQUIRE(out.size() == in.size());

        for (size_t i = 30000; i < out.size(); i++)
            REQUIRE(std::abs(ndsp_sig_test::toStd(out[i])) == Approx(1.0).margin(1e-3));
        // Gain only, no phase change
        CHECK(ndsp_sig_test::estimateFreq(out, 30000, out.size()) == Approx(0.1).epsilon(1e-4));
    }
}

TEST_CASE("ndsp AGCBlock<float> converges to the reference magnitude", "[dsp][agc]")
{
    std::vector<float> in(20000);
    for (size_t i = 0; i < in.size(); i++)
        in[i] = (i % 2) ? 0.2f : -0.2f;

    auto agc = std::make_shared<AGCBlock<float>>();
    configureAgc(*agc, 1e-2, 0.5f, 1.0f, 0);
    auto r = ndsp_sig_test::runBlock<float, float>(agc, in, ndsp_sig_test::chunked(1000));
    REQUIRE(r.completed);
    REQUIRE(r.output().size() == in.size());
    for (size_t i = 10000; i < in.size(); i++)
    {
        REQUIRE(std::fabs(r.output()[i]) == Approx(0.5f).margin(1e-4));
        REQUIRE((r.output()[i] > 0) == (in[i] > 0));
    }
}

TEST_CASE("ndsp AGCBlock limits the gain to max_gain", "[dsp][agc]")
{
    auto in = ndsp_sig_test::complexTone(20000, 0.0, 1e-3);
    auto agc = std::make_shared<AGCBlock<complex_t>>();
    configureAgc(*agc, 1e-1, 1.0f, 1.0f, 50.0f);
    auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(agc, in);
    REQUIRE(r.completed);
    // Would need a gain of 1000, stuck at 50
    for (size_t i = 10000; i < in.size(); i++)
        REQUIRE(std::abs(ndsp_sig_test::toStd(r.output()[i])) == Approx(0.05).epsilon(1e-3));
}

TEST_CASE("ndsp AGCBlock first sample uses the initial gain, and restarts reset the state", "[dsp][agc]")
{
    AGCBlock<complex_t> agc;
    configureAgc(agc, 0.1f, 1.0f, 2.5f, 65536);
    agc.init();
    std::vector<complex_t> in(4, complex_t(0.3f, 0.4f));
    auto out = ndsp_sig_test::processDirect<complex_t>(agc, in, 4);
    REQUIRE(out.size() == 4);
    CHECK(out[0].real == Approx(0.75f));
    CHECK(out[0].imag == Approx(1.0f));
    // |out0| = 1.25, gain -> 2.5 + 0.1 * (1 - 1.25) = 2.475
    CHECK(out[1].real == Approx(0.3f * 2.475f));

    // init() goes back to the configured gain
    agc.init();
    auto out2 = ndsp_sig_test::processDirect<complex_t>(agc, in, 4);
    CHECK(out2[0].real == Approx(0.75f));
}

TEST_CASE("ndsp AGCBlock configuration round-trip and IO", "[dsp][agc]")
{
    AGCBlock<complex_t> agcc;
    AGCBlock<float> agcf;
    CHECK(agcc.d_id == "agc_cc");
    CHECK(agcf.d_id == "agc_ff");
    REQUIRE(agcc.get_inputs().size() == 1);
    REQUIRE(agcc.get_outputs().size() == 1);
    CHECK(agcc.get_inputs()[0].type == DSP_SAMPLE_TYPE_CF32);
    CHECK(agcf.get_outputs()[0].type == DSP_SAMPLE_TYPE_F32);

    auto def = ndsp_sig_test::getAll(agcc);
    CHECK(def["rate"].get<float>() == Approx(1e-4));
    CHECK(def["reference"].get<float>() == Approx(1.0));
    CHECK(def["gain"].get<float>() == Approx(1.0));
    CHECK(def["max_gain"].get<float>() == Approx(65536));

    nlohmann::json cfg = {{"rate", 0.25}, {"reference", 0.7}, {"gain", 3.0}, {"max_gain", 12.0}};
    CHECK(ndsp_sig_test::setAll(agcc, cfg) == Block::RES_OK);
    auto got = ndsp_sig_test::getAll(agcc);
    for (auto &it : cfg.items())
        CHECK(got[it.key()].get<float>() == Approx(it.value().get<float>()));

    CHECK_THROWS(agcc.set_cfg("nope", 1));
    CHECK(ndsp_sig_test::setAll(agcc, {{"nope", 1}}) == Block::RES_ERR);
    CHECK_THROWS(agcc.get_cfg("nope"));
    // Wrong input type is rejected
    BlockIO bad;
    bad.type = DSP_SAMPLE_TYPE_F32;
    CHECK_THROWS(agcc.set_input(bad, 0));
    CHECK_THROWS(agcc.set_input(agcc.get_inputs()[0], 1));
}

TEST_CASE("ndsp AGCFastBlock<complex_t> matches AGCBlock and converges", "[dsp][agc]")
{
    ndsp_sig_test::Lcg rng(7);
    std::vector<complex_t> in(30000);
    for (size_t i = 0; i < in.size(); i++)
        in[i] = ndsp_sig_test::fromStd(std::polar(0.3 + 0.05 * rng.uniform(), 2 * ndsp_sig_test::kPi * rng.uniform()));

    auto slow = std::make_shared<AGCBlock<complex_t>>();
    auto fast = std::make_shared<AGCFastBlock<complex_t>>();
    configureAgc(*slow, 5e-3, 1.0f, 1.0f, 65536);
    configureAgc(*fast, 5e-3, 1.0f, 1.0f, 65536);
    auto rs = ndsp_sig_test::runBlock<complex_t, complex_t>(slow, in);
    auto rf = ndsp_sig_test::runBlock<complex_t, complex_t>(fast, in);
    REQUIRE(rs.completed);
    REQUIRE(rf.completed);
    REQUIRE(rf.output().size() == in.size());

    // Same update rule, so the outputs are the same up to float rounding
    for (size_t i = 0; i < in.size(); i++)
    {
        REQUIRE(rf.output()[i].real == Approx(rs.output()[i].real).margin(1e-3));
        REQUIRE(rf.output()[i].imag == Approx(rs.output()[i].imag).margin(1e-3));
    }
    // Average magnitude converges to the reference
    double avg = 0;
    for (size_t i = 20000; i < in.size(); i++)
        avg += std::abs(ndsp_sig_test::toStd(rf.output()[i]));
    CHECK(avg / 10000.0 == Approx(1.0).margin(0.02));
}

// Bug : agc_fast.cpp:39 computes the magnitudes of the float variant from out[i]
// (the not-yet-written output buffer) instead of in[i]. With a zeroed output buffer
// the AGC sees a zero magnitude and the gain runs away to max_gain.
TEST_CASE("ndsp AGCFastBlock<float> converges to the reference magnitude", "[dsp][agc][!shouldfail]")
{
    AGCFastBlock<float> agc;
    configureAgc(agc, 1e-2, 0.5f, 1.0f, 1000.0f);
    agc.init();

    std::vector<float> in(20000);
    for (size_t i = 0; i < in.size(); i++)
        in[i] = (i % 2) ? 0.2f : -0.2f;

    // Separate, zero-initialized output buffer, so the result is deterministic
    volk::vector<float> ibuf(in.begin(), in.end());
    volk::vector<float> obuf(in.size(), 0.0f);
    REQUIRE(agc.process(ibuf.data(), in.size(), obuf.data()) == in.size());
    for (size_t i = 10000; i < in.size(); i++)
        REQUIRE(std::fabs(obuf[i]) == Approx(0.5f).margin(1e-3));
}

TEST_CASE("ndsp AGCFastBlock configuration round-trip", "[dsp][agc]")
{
    AGCFastBlock<complex_t> agc;
    CHECK(agc.d_id == "agc_fast_cc");
    CHECK(AGCFastBlock<float>().d_id == "agc_fast_ff");
    nlohmann::json cfg = {{"rate", 0.125}, {"reference", 0.5}, {"gain", 2.0}, {"max_gain", 8.0}};
    CHECK(ndsp_sig_test::setAll(agc, cfg) == Block::RES_OK);
    auto got = ndsp_sig_test::getAll(agc);
    REQUIRE(got.size() == 4);
    for (auto &it : cfg.items())
        CHECK(got[it.key()].get<float>() == Approx(it.value().get<float>()));
    CHECK_THROWS(agc.set_cfg("bogus", 1));
    CHECK(ndsp_sig_test::setAll(agc, {{"bogus", 1}, {"rate", 0.5}}) == Block::RES_ERR);
    // Valid keys of a partially invalid set are still applied
    CHECK(agc.get_cfg("rate").get<float>() == Approx(0.5));
}

TEST_CASE("ndsp AGCFastBlock limits the gain to max_gain", "[dsp][agc]")
{
    auto in = ndsp_sig_test::complexTone(20000, 0.0, 1e-3);
    auto agc = std::make_shared<AGCFastBlock<complex_t>>();
    configureAgc(*agc, 1e-1, 1.0f, 1.0f, 50.0f);
    auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(agc, in);
    REQUIRE(r.completed);
    for (size_t i = 10000; i < in.size(); i++)
        REQUIRE(std::abs(ndsp_sig_test::toStd(r.output()[i])) == Approx(0.05).epsilon(1e-3));
}

namespace
{
    // Sine of amplitude 0.5 on top of a DC offset of 2, through an AGCBiasBlock
    std::vector<float> runAgcBias(size_t n)
    {
        std::vector<float> in(n);
        for (size_t i = 0; i < n; i++)
            in[i] = 2.0f + 0.5f * std::cos(0.05 * (double)i);

        auto agc = std::make_shared<AGCBiasBlock>();
        REQUIRE(ndsp_sig_test::setAll(*agc, nlohmann::json{{"target", 1.5}, {"bias", 0.01}, {"gain", 0.002}}) == Block::RES_OK);
        auto r = ndsp_sig_test::runBlock<float, float>(agc, in);
        REQUIRE(r.completed);
        REQUIRE(r.output().size() == n);
        return r.output();
    }

    // Whole periods of the tone (2pi / 0.05 ~ 125.66 samples) at the end
    const size_t kBiasN = 60000;
    const size_t kBiasStart = kBiasN - 25133;
} // namespace

TEST_CASE("ndsp AGCBiasBlock normalizes the mean magnitude to the target", "[dsp][agc]")
{
    auto out = runAgcBias(kBiasN);
    double mean_abs = 0;
    for (size_t i = kBiasStart; i < kBiasN; i++)
        mean_abs += std::fabs(out[i]);
    mean_abs /= (double)(kBiasN - kBiasStart);
    // The moving average tracks |x - bias|, so the output mean magnitude matches the target
    CHECK(mean_abs == Approx(1.5).epsilon(0.05));
    // The tone is still there
    CHECK(ndsp_sig_test::toneAmplitude(out, 0.05, kBiasStart, kBiasN) > 0.5);
}

// Bug : agc_bias.cpp:28 updates the bias estimate with the already de-biased sample
// (bias = bias * (1 - p) + (x - bias) * p), which converges to half of the DC offset,
// so only half of the DC bias is ever removed.
TEST_CASE("ndsp AGCBiasBlock removes the DC bias", "[dsp][agc][!shouldfail]")
{
    auto out = runAgcBias(kBiasN);
    double mean = 0;
    for (size_t i = kBiasStart; i < kBiasN; i++)
        mean += out[i];
    mean /= (double)(kBiasN - kBiasStart);
    CHECK(std::fabs(mean) < 0.05);
}

// Bug : agc_bias.cpp:25-26 returns 0 (drops the whole output buffer, including the
// already processed samples) as soon as a single input sample is exactly 0.
TEST_CASE("ndsp AGCBiasBlock keeps processing buffers that contain a zero sample", "[dsp][agc][!shouldfail]")
{
    AGCBiasBlock agc;
    agc.init();
    std::vector<float> in(100, 1.0f);
    in[50] = 0.0f;
    auto out = ndsp_sig_test::processDirect<float>(agc, in, in.size());
    CHECK(out.size() == in.size());
}

TEST_CASE("ndsp AGCBiasBlock configuration round-trip", "[dsp][agc]")
{
    AGCBiasBlock agc;
    CHECK(agc.d_id == "agc_bias");
    auto def = ndsp_sig_test::getAll(agc);
    CHECK(def["target"].get<float>() == Approx(5));
    CHECK(def["bias"].get<float>() == Approx(0.01));
    CHECK(def["gain"].get<float>() == Approx(0.001));

    nlohmann::json cfg = {{"target", 2.0}, {"bias", 0.5}, {"gain", 0.25}};
    CHECK(ndsp_sig_test::setAll(agc, cfg) == Block::RES_OK);
    auto got = ndsp_sig_test::getAll(agc);
    for (auto &it : cfg.items())
        CHECK(got[it.key()].get<float>() == Approx(it.value().get<float>()));
    CHECK_THROWS(agc.set_cfg("x", 0));
    CHECK(ndsp_sig_test::setAll(agc, {{"x", 0}}) == Block::RES_ERR);
    CHECK_THROWS(agc.get_cfg("x"));
}
