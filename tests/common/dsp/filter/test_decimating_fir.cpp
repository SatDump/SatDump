#include "common/dsp/dsp_test_helpers.h"
#include "common/dsp/filter/decimating_fir.h"
#include "common/dsp/filter/firdes.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

namespace
{
    // Feeds the input through process() in chunks and concatenates the output
    template <typename T>
    std::vector<T> processChunked(dsp::DecimatingFIRBlock<T> &fir, const std::vector<T> &in, size_t chunk)
    {
        std::vector<T> out;
        dsp_test::VolkBuffer<T> ibuf(chunk), obuf(chunk);
        for (size_t pos = 0; pos < in.size(); pos += chunk)
        {
            int n = std::min(chunk, in.size() - pos);
            memcpy((void *)ibuf.data(), &in[pos], n * sizeof(T));
            int outn = fir.process(ibuf.data(), n, obuf.data());
            out.insert(out.end(), obuf.data(), obuf.data() + outn);
        }
        return out;
    }

    std::vector<complex_t> randomComplex(size_t n, uint32_t seed)
    {
        dsp_test::Lcg rng(seed);
        std::vector<complex_t> v(n);
        for (auto &x : v)
            x = complex_t(rng.uniform() * 2 - 1, rng.uniform() * 2 - 1);
        return v;
    }
} // namespace

TEST_CASE("DecimatingFIRBlock::process equals filtering then keeping every Dth sample", "[common][dsp][decimating_fir]")
{
    auto taps = dsp::firdes::low_pass(1.0, 1.0, 0.1, 0.05);
    auto in = randomComplex(12000, 10);
    auto full = dsp_test::referenceFir(in, taps);

    for (int decim : {2, 3, 7})
    {
        for (size_t chunk : {(size_t)1000, (size_t)999, (size_t)12000})
        {
            INFO("decim = " << decim << ", chunk = " << chunk);
            dsp::DecimatingFIRBlock<complex_t> fir(nullptr, taps, decim);
            auto out = processChunked(fir, in, chunk);

            // Output index k corresponds to input index k * decim
            REQUIRE(out.size() == (in.size() + decim - 1) / decim);
            size_t mismatches = 0, first = 0;
            for (size_t k = 0; k < out.size(); k++)
                if (!(dsp_test::absDiff(out[k], full[k * decim]) < 1e-5f) && mismatches++ == 0)
                    first = k;
            INFO("first mismatch at sample " << first);
            REQUIRE(mismatches == 0);
        }
    }
}

TEST_CASE("DecimatingFIRBlock<float> decimation ratio and DC gain", "[common][dsp][decimating_fir]")
{
    auto taps = dsp::firdes::low_pass(1.0, 1.0, 0.05, 0.02);
    dsp::DecimatingFIRBlock<float> fir(nullptr, taps, 4);

    std::vector<float> in(40000, 0.75f);
    auto out = processChunked(fir, in, 4096);
    REQUIRE(out.size() == 10000);

    // Constant input settles to DC gain * value
    size_t mismatches = 0, first = 0;
    for (size_t k = taps.size(); k < out.size(); k++)
        if (!(out[k] == Approx(0.75f).margin(1e-4)) && mismatches++ == 0)
            first = k;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}

TEST_CASE("DecimatingFIRBlock preserves in-band tone frequency in Hz", "[common][dsp][decimating_fir]")
{
    const double fs = 1e6;
    const int decim = 5;
    auto taps = dsp::firdes::low_pass(1.0, fs, 80e3, 20e3);
    dsp::DecimatingFIRBlock<complex_t> fir(nullptr, taps, decim);

    auto in = dsp_test::complexTone(100000, dsp::hz_to_rad(30e3, fs));
    auto out = processChunked(fir, in, 8192);
    REQUIRE(out.size() == 20000);

    // Same 30 kHz tone, now at fs / decim
    double f = dsp_test::estimateFreq(out, taps.size(), out.size());
    CHECK(dsp::rad_to_hz(f, fs / decim) == Approx(30e3).epsilon(1e-4));
    CHECK(dsp_test::rms(out, taps.size(), out.size()) == Approx(1.0).margin(0.01));
}

TEST_CASE("DecimatingFIRBlock rejects tones that would alias", "[common][dsp][decimating_fir]")
{
    const double fs = 1e6;
    auto taps = dsp::firdes::low_pass(1.0, fs, 80e3, 20e3, dsp::fft::window::WIN_BLACKMAN);
    dsp::DecimatingFIRBlock<complex_t> fir(nullptr, taps, 5);

    // 170 kHz would alias to -30 kHz after decimation to 200 kHz
    auto out = processChunked(fir, dsp_test::complexTone(100000, dsp::hz_to_rad(170e3, fs)), 8192);
    CHECK(dsp_test::toDb(dsp_test::rms(out, taps.size(), out.size())) < -60.0);
}

TEST_CASE("DecimatingFIRBlock as a streaming block", "[common][dsp][decimating_fir]")
{
    auto taps = dsp::firdes::low_pass(1.0, 1.0, 0.1, 0.05);
    auto in = randomComplex(30000, 11);
    auto full = dsp_test::referenceFir(in, taps);

    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::DecimatingFIRBlock<complex_t> fir(input, taps, 3);
    auto r = dsp_test::runBlock(fir, input, in, 1001);
    REQUIRE(r.completed);
    REQUIRE(r.output.size() == 10000);
    size_t mismatches = 0, first = 0;
    for (size_t k = 0; k < r.output.size(); k++)
        if (!(dsp_test::absDiff(r.output[k], full[k * 3]) < 1e-5f) && mismatches++ == 0)
            first = k;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}
