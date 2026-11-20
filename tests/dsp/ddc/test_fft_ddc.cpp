#include "dsp/ddc/fft_ddc.h"
#include "dsp/ndsp_sig_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using Catch::Approx;
using namespace satdump::ndsp;

namespace
{
    double hzToRadF(double hz, double fs) { return 2.0 * M_PI * hz / fs; }
} // namespace

TEST_CASE("ndsp FFTDDCBlock filters, shifts and decimates a VFO", "[dsp][ddc][fft_ddc]")
{
    const double fs = 1e6, cf = 100e6;
    auto ddc = std::make_shared<FFTDDCBlock>();
    REQUIRE(ndsp_sig_test::setAll(*ddc, {{"frequency", cf}, {"samplerate", fs}}) == Block::RES_OK);
    // 50 kHz wide VFO at +100 kHz, decimated by 4, plus a passthrough output
    ddc->add_output("vfo", cf + 100e3, 50e3, 4);
    ddc->add_output("raw", 0, 0, 1);

    // In band tone (+10 kHz from the VFO center), and out of band tones
    const size_t n = 200000;
    std::vector<complex_t> in(n, complex_t(0, 0));
    for (auto t : {std::make_pair(110e3, 1.0), std::make_pair(300e3, 1.0), std::make_pair(150e3, 1.0)})
    {
        auto s = ndsp_sig_test::complexTone(n, hzToRadF(t.first, fs), t.second);
        for (size_t i = 0; i < n; i++)
            in[i] = in[i] + s[i];
    }

    ndsp_sig_test::RunOptions opt;
    opt.out_indices = {0, 1};
    opt.chunk = 8192;
    auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(ddc, in, opt);
    REQUIRE(r.completed);
    auto &vfo = r.outputs[0].data;
    auto &raw = r.outputs[1].data;

    // The passthrough output is an exact copy
    CHECK(ndsp_sig_test::sameSamples(raw, in));

    // Processed in FFT blocks, the remainder stays buffered
    CHECK(vfo.size() <= n / 4);
    CHECK(vfo.size() >= (n - 40000) / 4);

    // Skip the filter delay (7200 input samples)
    size_t s = 2000;
    double fo = fs / 4;
    CHECK(ndsp_sig_test::toneAmplitude(vfo, hzToRadF(10e3, fo), s, vfo.size()) == Approx(1.0).epsilon(0.01));
    CHECK(ndsp_sig_test::estimateFreq(vfo, s, vfo.size()) == Approx(hzToRadF(10e3, fo)).epsilon(1e-3));
    // 300 kHz -> 200 kHz from the VFO, aliases to -50 kHz after decimation. 150 kHz -> +50 kHz
    CHECK(ndsp_sig_test::toDb(ndsp_sig_test::toneAmplitude(vfo, hzToRadF(-50e3, fo), s, vfo.size())) < -45);
    CHECK(ndsp_sig_test::toDb(ndsp_sig_test::toneAmplitude(vfo, hzToRadF(50e3, fo), s, vfo.size())) < -45);
}

TEST_CASE("ndsp FFTDDCBlock VFO frequency change retunes the filter", "[dsp][ddc][fft_ddc]")
{
    const double fs = 1e6, cf = 0;
    auto ddc = std::make_shared<FFTDDCBlock>();
    REQUIRE(ndsp_sig_test::setAll(*ddc, {{"frequency", cf}, {"samplerate", fs}}) == Block::RES_OK);
    ddc->add_output("vfo", 100e3, 40e3, 2);
    ddc->set_vfo_freq("vfo", -200e3);
    CHECK(ddc->get_vfo_freq("vfo") == -200e3);

    const size_t n = 120000;
    auto in = ndsp_sig_test::complexTone(n, hzToRadF(-205e3, fs));
    auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(ddc, in, ndsp_sig_test::chunked(10000));
    REQUIRE(r.completed);
    auto &out = r.output();
    REQUIRE(out.size() > 20000);
    CHECK(ndsp_sig_test::estimateFreq(out, 5000, out.size()) == Approx(hzToRadF(-5e3, fs / 2)).epsilon(1e-3));
    CHECK(ndsp_sig_test::toneAmplitude(out, hzToRadF(-5e3, fs / 2), 5000, out.size()) == Approx(1.0).epsilon(0.01));
}

TEST_CASE("ndsp FFTDDCBlock configuration and VFO management", "[dsp][ddc][fft_ddc]")
{
    FFTDDCBlock ddc;
    CHECK(ddc.d_id == "fft_ddc_c");
    REQUIRE(ndsp_sig_test::setAll(ddc, {{"frequency", 137e6}, {"samplerate", 2e6}}) == Block::RES_OK);
    ddc.add_output("a", 137.1e6, 30e3, 4);
    ddc.add_output("b", 136.8e6, 60e3, 8);

    auto list = ddc.get_cfg_list();
    for (auto k : {"frequency", "samplerate", "noutputs", "output_0_freq", "output_0_bw", "output_0_dec", "output_1_dec"})
        CHECK(list.contains(k));

    CHECK(ddc.get_cfg("output_0_freq") == 137.1e6);
    CHECK(ddc.get_cfg("output_1_bw") == 60e3);
    CHECK(ddc.get_cfg("output_1_dec") == 8);
    CHECK(ddc.set_cfg("output_1_freq", 136.7e6) == Block::RES_OK);
    CHECK(ddc.set_cfg("output_1_bw", 10e3) == Block::RES_OK);
    CHECK(ddc.set_cfg("output_1_dec", 3) == Block::RES_OK);
    CHECK(ddc.get_vfo_freq("b") == 136.7e6);
    CHECK(ddc.get_vfo_bandwidth("b") == 10e3);
    CHECK(ddc.get_vfo_dec("b") == 3);
    // Decimation is clamped to 1 through the config (but not through set_vfo_dec)
    CHECK(ddc.set_cfg("output_1_dec", 0) == Block::RES_OK);
    CHECK(ddc.get_vfo_dec("b") == 1);

    ddc.set_vfo_bandwidth("a", 12e3);
    CHECK(ddc.get_cfg("output_0_bw") == 12e3);
    CHECK(ddc.get_vfo_freq("zz") == -1);
    CHECK(ddc.get_vfo_bandwidth("zz") == -1);
    CHECK(ddc.get_vfo_dec("zz") == -1);
    CHECK_THROWS(ddc.get_output_by_id("zz"));
    ddc.del_output("a", false);
    REQUIRE(ddc.get_outputs().size() == 1);
    CHECK(ddc.get_output_by_id("b").name == "b");
    CHECK_THROWS(ddc.set_cfg("nope", 1));
    CHECK_THROWS(ddc.get_cfg("nope"));
}
