#include "common/dsp/filter/firdes.h"
#include "dsp/filter/fft.h"
#include "dsp/filter/lpf.h"
#include "dsp/filter/notch_bank.h"
#include "ndsp_sig_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;
using namespace satdump::ndsp;

namespace
{
    // Causal convolution with zero initial state, y[n] = sum_k h[k] x[n - k]
    template <typename TT>
    std::complex<double> causalConv(const std::vector<complex_t> &x, const std::vector<TT> &h, size_t n)
    {
        std::complex<double> acc = 0;
        for (size_t k = 0; k < h.size() && k <= n; k++)
        {
            std::complex<double> hk;
            if constexpr (std::is_same_v<TT, complex_t>)
                hk = ndsp_sig_test::toStd(h[k]);
            else
                hk = h[k];
            acc += hk * ndsp_sig_test::toStd(x[n - k]);
        }
        return acc;
    }

    std::vector<complex_t> randomComplex(size_t n, uint32_t seed)
    {
        ndsp_sig_test::Lcg rng(seed);
        std::vector<complex_t> t(n);
        for (auto &v : t)
            v = complex_t(rng.symmetric(), rng.symmetric());
        return t;
    }

    template <typename TT>
    void checkFFTFilter(const std::vector<TT> &taps, const std::vector<complex_t> &in, size_t chunk)
    {
        auto blk = std::make_shared<FFTFilterBlock<complex_t, TT>>();
        REQUIRE(blk->set_cfg("taps", taps) == Block::RES_OK);
        auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(blk, in, ndsp_sig_test::chunked(chunk));
        REQUIRE(r.completed);
        auto &out = r.output();

        // Output comes in blocks of a fixed size, the remainder is kept internally
        REQUIRE(!r.buffer_sizes[0].empty());
        size_t bs = r.buffer_sizes[0][0];
        for (auto s : r.buffer_sizes[0])
            REQUIRE(s == bs);
        CHECK(bs >= taps.size());
        CHECK(in.size() - out.size() <= bs);

        // Overlap-add, so exactly a linear convolution
        for (size_t i = 0; i < out.size(); i += 7)
        {
            INFO("sample " << i);
            auto ref = causalConv(in, taps, i);
            REQUIRE(out[i].real == Approx(ref.real()).margin(1e-3));
            REQUIRE(out[i].imag == Approx(ref.imag()).margin(1e-3));
        }
    }
} // namespace

TEST_CASE("ndsp FFTFilterBlock<complex_t> matches a causal convolution", "[dsp][filter][fft_filter]")
{
    ndsp_sig_test::Lcg rng(1);
    std::vector<float> taps(13);
    for (auto &t : taps)
        t = rng.symmetric();
    checkFFTFilter(taps, randomComplex(30000, 2), 4096);
}

TEST_CASE("ndsp FFTFilterBlock<complex_t, complex_t> matches a causal convolution", "[dsp][filter][fft_filter]")
{
    checkFFTFilter(randomComplex(40, 3), randomComplex(40000, 4), 3000);
}

TEST_CASE("ndsp FFTFilterBlock as LPF_Block filters tones", "[dsp][filter][fft_filter]")
{
    const double fs = 1e6;
    auto lpf = std::make_shared<LPF_Block<FFTFilterBlock<complex_t>>>();
    REQUIRE(ndsp_sig_test::setAll(*lpf, {{"samplerate", fs}, {"cutoff", 100e3}, {"transition_width", 50e3}}) == Block::RES_OK);
    CHECK(lpf->d_id == "lpffft_filter__cc");

    const size_t n = 60000;
    double w_in = 2 * ndsp_sig_test::kPi * -40e3 / fs, w_out = 2 * ndsp_sig_test::kPi * 300e3 / fs;
    auto a = ndsp_sig_test::complexTone(n, w_in, 1.0);
    auto b = ndsp_sig_test::complexTone(n, w_out, 1.0);
    for (size_t i = 0; i < n; i++)
        a[i] = a[i] + b[i];
    auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(lpf, a);
    REQUIRE(r.completed);
    auto &out = r.output();
    REQUIRE(out.size() > 10000);
    CHECK(ndsp_sig_test::toneAmplitude(out, w_in, 1000, out.size()) == Approx(1.0).epsilon(0.01));
    CHECK(ndsp_sig_test::toDb(ndsp_sig_test::toneAmplitude(out, w_out, 1000, out.size())) < -40);
}

TEST_CASE("ndsp FFTFilterBlock configuration round-trip", "[dsp][filter][fft_filter]")
{
    FFTFilterBlock<complex_t> f;
    CHECK(f.d_id == "fft_filter__cc");
    CHECK(FFTFilterBlock<complex_t, complex_t>().d_id == "fft_filter_c_cc");
    std::vector<float> t = {1, -2, 0.5};
    CHECK(f.set_cfg("taps", t) == Block::RES_OK);
    CHECK(f.needs_reinit);
    CHECK(f.get_cfg("taps").get<std::vector<float>>() == t);
    CHECK_THROWS(f.set_cfg("x", 1));
    CHECK_THROWS(f.get_cfg("x"));
}

namespace
{
    std::shared_ptr<NotchBankBlock> makeNotch(const std::vector<double> &freqs, double fs)
    {
        auto nb = std::make_shared<NotchBankBlock>();
        REQUIRE(nb->set_cfg("frequency", 100e6) == Block::RES_OK);
        REQUIRE(nb->set_cfg("samplerate", fs) == Block::RES_OK);
        REQUIRE(nb->set_cfg("notch_cnt", (int)freqs.size()) == Block::RES_LISTUPD);
        for (size_t i = 0; i < freqs.size(); i++)
        {
            std::string p = "notch_" + std::to_string(i + 1);
            REQUIRE(nb->set_cfg(p + "_freq", freqs[i]) == Block::RES_OK);
            REQUIRE(nb->set_cfg(p + "_bw", 3000.0) == Block::RES_OK);
            REQUIRE(nb->set_cfg(p + "_tr", 2000.0) == Block::RES_OK);
        }
        return nb;
    }
} // namespace

TEST_CASE("ndsp NotchBankBlock removes tones at the notch frequencies only", "[dsp][filter][fft_filter]")
{
    const double fs = 100e3;
    // Notches at +20 kHz and -30 kHz relative to the 100 MHz center
    auto nb = makeNotch({100.02e6, 99.97e6}, fs);

    auto w = [&](double hz) { return 2 * ndsp_sig_test::kPi * hz / fs; };
    const size_t n = 180000;
    std::vector<complex_t> in(n, complex_t(0, 0));
    for (double f : {20e3, -30e3, 10e3, -45e3})
    {
        auto t = ndsp_sig_test::complexTone(n, w(f), 1.0);
        for (size_t i = 0; i < n; i++)
            in[i] = in[i] + t[i];
    }

    auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(nb, in, ndsp_sig_test::chunked(8192));
    REQUIRE(r.completed);
    auto &out = r.output();
    REQUIRE(out.size() > 50000);
    size_t s = 2000;
    CHECK(ndsp_sig_test::toDb(ndsp_sig_test::toneAmplitude(out, w(20e3), s, out.size())) < -20);
    CHECK(ndsp_sig_test::toDb(ndsp_sig_test::toneAmplitude(out, w(-30e3), s, out.size())) < -20);
    CHECK(ndsp_sig_test::toneAmplitude(out, w(10e3), s, out.size()) == Approx(1.0).margin(0.05));
    CHECK(ndsp_sig_test::toneAmplitude(out, w(-45e3), s, out.size()) == Approx(1.0).margin(0.05));
}

TEST_CASE("ndsp NotchBankBlock configuration", "[dsp][filter][fft_filter]")
{
    NotchBankBlock nb;
    CHECK(nb.d_id == "notch_bankfft_filter_c_cc");
    auto list = nb.get_cfg_list();
    CHECK(list.contains("notch_1_freq"));
    CHECK(!list.contains("notch_2_freq"));

    // Setting a notch past the current count grows the list
    CHECK(nb.set_cfg("notch_3_bw", 1234.0) == Block::RES_LISTUPD);
    CHECK(nb.get_cfg("notch_cnt") == 3);
    list = nb.get_cfg_list();
    CHECK(list.contains("notch_3_tr"));
    CHECK(nb.get_cfg("notch_3_bw").get<double>() == 1234.0);
    // Defaults of the new notches
    CHECK(nb.get_cfg("notch_2_freq").get<double>() == 101e6);
    CHECK(nb.get_cfg("notch_2_bw").get<double>() == 5000);
    CHECK(nb.get_cfg("notch_2_tr").get<double>() == 2000);

    // Clamped to at least 1 notch
    CHECK(nb.set_cfg("notch_cnt", 0) == Block::RES_LISTUPD);
    CHECK(nb.get_cfg("notch_cnt") == 1);

    nlohmann::json cfg = {{"frequency", 137e6}, {"samplerate", 2e5}, {"notch_cnt", 1}, {"notch_1_freq", 137.01e6}, {"notch_1_bw", 2000.0}, {"notch_1_tr", 1000.0}};
    ndsp_sig_test::setAll(nb, cfg);
    CHECK(ndsp_sig_test::getAll(nb) == cfg);

    // Taps : combination of the shifted high-pass filters
    auto taps = nb.get_cfg("taps");
    auto hp = dsp::firdes::high_pass(1, 2e5, 2000.0, 1000.0);
    CHECK(taps.size() == hp.size());
}
