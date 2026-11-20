#include "dsp/ddc/ddc.h"
#include "dsp/ndsp_sig_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using Catch::Approx;
using namespace satdump::ndsp;

namespace
{
    double hzToRad(double hz, double fs) { return 2.0 * M_PI * hz / fs; }

    std::vector<complex_t> sumTones(size_t n, const std::vector<std::pair<double, double>> &tones /* rad, amp */)
    {
        std::vector<complex_t> v(n, complex_t(0, 0));
        for (auto &t : tones)
        {
            auto s = ndsp_sig_test::complexTone(n, t.first, t.second);
            for (size_t i = 0; i < n; i++)
                v[i] = v[i] + s[i];
        }
        return v;
    }
} // namespace

namespace
{
    // Two VFOs : +100 kHz without decimation, -250 kHz decimated by 4
    void checkTwoVfos(bool retune_after_add)
    {
        const double fs = 1e6, cf = 100e6;
        auto ddc = std::make_shared<DDC_Block>();
        REQUIRE(ndsp_sig_test::setAll(*ddc, {{"frequency", cf}, {"samplerate", fs}}) == Block::RES_OK);
        // No bandwidth : frequency shift (+ decimation by picking samples) only
        ddc->add_output("a", cf + 100e3, 0, 1);
        ddc->add_output("b", cf - 250e3, 0, 4);
        if (retune_after_add)
            REQUIRE(ddc->set_cfg("frequency", cf) == Block::RES_OK);

        const size_t n = 40000;
        auto in = sumTones(n, {{hzToRad(100e3, fs), 1.0}, {hzToRad(-240e3, fs), 0.5}});
        ndsp_sig_test::RunOptions opt;
        opt.out_indices = {0, 1};
        auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(ddc, in, opt);
        REQUIRE(r.completed);
        auto &a = r.outputs[0].data;
        auto &b = r.outputs[1].data;
        REQUIRE(a.size() == n);
        REQUIRE(b.size() == n / 4);

        // a : the +100 kHz tone is now at DC
        CHECK(ndsp_sig_test::toneAmplitude(a, 0.0, 0, a.size()) == Approx(1.0).epsilon(1e-3));
        CHECK(ndsp_sig_test::toneAmplitude(a, hzToRad(-340e3, fs), 0, a.size()) == Approx(0.5).epsilon(1e-3));
        // b : the -240 kHz tone is now at +10 kHz, at a quarter of the sample rate
        CHECK(ndsp_sig_test::toneAmplitude(b, hzToRad(10e3, fs / 4), 0, b.size()) == Approx(0.5).epsilon(1e-3));
    }
} // namespace

TEST_CASE("ndsp DDC_Block shifts each VFO to baseband and decimates", "[dsp][ddc]")
{
    // Retuning after adding the VFOs computes their NCO, see the test below
    checkTwoVfos(true);
}

// Bug: DDC_Block::add_output (ddc.h:251-259) never calls recalc_freq() / recalc_bw() on
// the new VFO (unlike FFTDDCBlock::add_output), so its NCO phase and phase increment stay 0
// and the rotator outputs NaNs until frequency / samplerate / the VFO frequency is set again.
TEST_CASE("ndsp DDC_Block VFOs work right after add_output", "[dsp][ddc][!shouldfail]") { checkTwoVfos(false); }

TEST_CASE("ndsp DDC_Block decimation without frequency shift picks every Nth sample", "[dsp][ddc]")
{
    // VFO at the absolute frequency 0 is a plain copy
    auto ddc = std::make_shared<DDC_Block>();
    REQUIRE(ndsp_sig_test::setAll(*ddc, {{"frequency", 0.0}, {"samplerate", 1e6}}) == Block::RES_OK);
    ddc->add_output("a", 0, 0, 3);
    std::vector<complex_t> in(1000);
    for (size_t i = 0; i < in.size(); i++)
        in[i] = complex_t(i, -(float)i);
    // Buffer size not a multiple of the decimation, the phase must carry over
    auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(ddc, in, ndsp_sig_test::chunked(100));
    REQUIRE(r.completed);
    auto &out = r.output();
    REQUIRE(out.size() == 333);
    for (size_t k = 0; k < out.size(); k++)
    {
        REQUIRE(out[k].real == 3 * k + 2);
        REQUIRE(out[k].imag == -(float)(3 * k + 2));
    }
}

TEST_CASE("ndsp DDC_Block VFO frequency can be changed through the config", "[dsp][ddc]")
{
    const double fs = 2e6, cf = 435e6;
    auto ddc = std::make_shared<DDC_Block>();
    REQUIRE(ndsp_sig_test::setAll(*ddc, {{"frequency", cf}, {"samplerate", fs}}) == Block::RES_OK);
    CHECK(ddc->set_cfg("noutputs", 1) == Block::RES_IOUPD);
    REQUIRE(ddc->set_cfg("output_0_freq", cf - 300e3) == Block::RES_OK);

    auto in = ndsp_sig_test::complexTone(20000, hzToRad(-280e3, fs));
    auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(ddc, in);
    REQUIRE(r.completed);
    CHECK(ndsp_sig_test::estimateFreq(r.output(), 0, r.output().size()) == Approx(hzToRad(20e3, fs)).epsilon(1e-4));
}

// Bug: the DDC_Block filter path relies on DecimatingFIRBlock, which does not decimate
// correctly (decimating_fir.cpp:120, see the filter tests), so a VFO with a bandwidth
// does not output samples at samplerate / decimation.
TEST_CASE("ndsp DDC_Block with a bandwidth filters and decimates", "[dsp][ddc][!shouldfail]")
{
    const double fs = 1e6, cf = 100e6;
    auto ddc = std::make_shared<DDC_Block>();
    REQUIRE(ndsp_sig_test::setAll(*ddc, {{"frequency", cf}, {"samplerate", fs}}) == Block::RES_OK);
    ddc->add_output("a", cf, 0, 4);
    ddc->set_vfo_freq("a", cf); // Computes the NCO (see above)
    ddc->set_vfo_bandwidth("a", 100e3);

    const size_t n = 40000;
    auto in = sumTones(n, {{hzToRad(20e3, fs), 1.0}, {hzToRad(300e3, fs), 1.0}});
    auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(ddc, in);
    REQUIRE(r.completed);
    auto &out = r.output();
    CHECK(std::abs((double)out.size() - n / 4.0) < 1000);
    REQUIRE(out.size() > 2000);
    CHECK(ndsp_sig_test::toneAmplitude(out, hzToRad(20e3, fs / 4), 1000, out.size()) == Approx(1.0).epsilon(0.02));
}

// Bug: ddc.h:56 sizes the taps vector with len * sizeof(double) instead of len,
// so a 601-taps filter ends up 4808 taps long (zero padded), 8x the work.
TEST_CASE("ndsp DDC_Block VFO filter has 2M+1 taps", "[dsp][ddc][!shouldfail]")
{
    DDC_Block ddc;
    REQUIRE(ndsp_sig_test::setAll(ddc, {{"frequency", 100e6}, {"samplerate", 1e6}}) == Block::RES_OK);
    ddc.add_output("a", 100e6, 0, 1);
    ddc.set_vfo_bandwidth("a", 50e3);
    auto *io = (DDC_Block::IOInfo *)ddc.get_outputs()[0].blkdata.get();
    CHECK(io->filter.get_cfg("taps").size() == 601);
}

TEST_CASE("ndsp DDC_Block VFO filter taps are a low-pass at bandwidth / 2", "[dsp][ddc]")
{
    DDC_Block ddc;
    REQUIRE(ndsp_sig_test::setAll(ddc, {{"frequency", 100e6}, {"samplerate", 1e6}}) == Block::RES_OK);
    ddc.add_output("a", 100e6, 0, 1);
    ddc.set_vfo_bandwidth("a", 100e3);
    auto *io = (DDC_Block::IOInfo *)ddc.get_outputs()[0].blkdata.get();
    auto taps = io->filter.get_cfg("taps").get<std::vector<float>>();
    REQUIRE(taps.size() >= 601);
    // Leading 601 taps : Kaiser-windowed sinc, symmetric, unity DC gain
    std::vector<float> t(taps.begin(), taps.begin() + 601);
    for (size_t i = 0; i < 300; i++)
        REQUIRE(t[i] == Approx(t[600 - i]).margin(1e-7));
    CHECK(ndsp_sig_test::magResponse(t, 0.0) == Approx(1.0).epsilon(1e-3));
    CHECK(ndsp_sig_test::magResponse(t, hzToRad(20e3, 1e6)) == Approx(1.0).epsilon(1e-3));
    CHECK(ndsp_sig_test::toDb(ndsp_sig_test::magResponse(t, hzToRad(80e3, 1e6))) < -70);
}

TEST_CASE("ndsp DDC_Block configuration and VFO management", "[dsp][ddc]")
{
    DDC_Block ddc;
    CHECK(ddc.d_id == "ddc_c");
    CHECK(ddc.get_outputs().empty());
    CHECK(ddc.set_cfg("noutputs", 2) == Block::RES_IOUPD);
    REQUIRE(ddc.get_outputs().size() == 2);
    CHECK(ddc.get_outputs()[1].name == "out2");

    auto list = ddc.get_cfg_list();
    for (auto k : {"frequency", "samplerate", "noutputs", "output_0_freq", "output_0_bw", "output_0_dec", "output_1_freq", "output_1_bw", "output_1_dec"})
        CHECK(list.contains(k));
    CHECK(list["noutputs"]["disable"] == false);

    nlohmann::json cfg = {{"frequency", 137e6}, {"samplerate", 2.4e6},      {"noutputs", 2},      {"output_0_freq", 137.1e6}, {"output_0_bw", 0.0},
                          {"output_0_dec", 2},  {"output_1_freq", 136.9e6}, {"output_1_bw", 0.0}, {"output_1_dec", 5}};
    ndsp_sig_test::setAll(ddc, cfg);
    CHECK(ndsp_sig_test::getAll(ddc) == cfg);

    // VFOs created through noutputs are named after their index
    CHECK(ddc.get_vfo_freq("1") == 137.1e6);
    CHECK(ddc.get_vfo_dec("2") == 5);
    ddc.set_vfo_freq("2", 137.5e6);
    ddc.set_vfo_dec("2", 7);
    CHECK(ddc.get_cfg("output_1_freq") == 137.5e6);
    CHECK(ddc.get_cfg("output_1_dec") == 7);
    CHECK(ddc.get_vfo_freq("nope") == -1);
    CHECK(ddc.get_vfo_bandwidth("nope") == -1);
    CHECK(ddc.get_vfo_dec("nope") == -1);

    ddc.add_output("extra", 137e6, 0, 1);
    CHECK(ddc.get_output_by_id("extra").name == "extra");
    CHECK_THROWS(ddc.get_output_by_id("nope"));
    ddc.del_output("1", false);
    REQUIRE(ddc.get_outputs().size() == 2);
    CHECK_THROWS(ddc.get_output_by_id("1"));

    CHECK_THROWS(ddc.get_cfg("output_0_freqx"));
    CHECK_THROWS(ddc.set_cfg("whatever", 1));
}

TEST_CASE("ndsp DDC_Block forwards terminators only to the outputs asking for it", "[dsp][ddc]")
{
    auto ddc = std::make_shared<DDC_Block>();
    REQUIRE(ndsp_sig_test::setAll(*ddc, {{"frequency", 0.0}, {"samplerate", 1e6}}) == Block::RES_OK);
    ddc->add_output("fwd", 0, 0, 1, true);
    ddc->add_output("nofwd", 0, 0, 1, false);
    std::vector<complex_t> in(100, complex_t(1, 0));
    // Only collect the forwarding one, the other would never terminate
    auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(ddc, in);
    REQUIRE(r.completed);
    CHECK(r.output().size() == 100);
    // The other one got the samples, but no terminator
    auto f = ddc->get_outputs()[1].fifo;
    size_t got = 0;
    bool term = false;
    DSPBuffer b;
    while (f->try_dequeue(b))
    {
        if (b.isTerminator())
            term = true;
        else
            got += b.size;
        f->free(b);
    }
    CHECK(got == 100);
    CHECK_FALSE(term);
}
