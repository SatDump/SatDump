#include "common/dsp/dsp_test_helpers.h"
#include "common/dsp/resamp/power_decim.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using Catch::Approx;

namespace
{
    template <typename T>
    std::vector<T> processChunked(dsp::PowerDecimatorBlock<T> &dec, const std::vector<T> &in, size_t chunk)
    {
        std::vector<T> out;
        dsp_test::VolkBuffer<T> ibuf(chunk), obuf(chunk);
        for (size_t pos = 0; pos < in.size(); pos += chunk)
        {
            int n = std::min(chunk, in.size() - pos);
            memcpy((void *)ibuf.data(), &in[pos], n * sizeof(T));
            int outn = dec.process(ibuf.data(), n, obuf.data());
            out.insert(out.end(), obuf.data(), obuf.data() + outn);
        }
        return out;
    }
} // namespace

TEST_CASE("PowerDecimatorBlock::max_decim is 2^13", "[common][dsp][power_decim]")
{
    CHECK(dsp::PowerDecimatorBlock<complex_t>::max_decim() == 8192);
    CHECK(dsp::PowerDecimatorBlock<float>::max_decim() == 8192);
}

TEST_CASE("PowerDecimatorBlock rejects non power of 2 decimations", "[common][dsp][power_decim]")
{
    for (unsigned d : {3u, 6u, 12u, 100u, 1000u})
    {
        INFO("decim = " << d);
        CHECK_THROWS(dsp::PowerDecimatorBlock<complex_t>(nullptr, d));
    }
}

TEST_CASE("PowerDecimatorBlock decimation 1 is a passthrough", "[common][dsp][power_decim]")
{
    dsp::PowerDecimatorBlock<float> dec(nullptr, 1);
    std::vector<float> in(1000);
    for (size_t i = 0; i < in.size(); i++)
        in[i] = (float)i * 0.5f;
    auto out = processChunked(dec, in, 333);
    CHECK(out == in);
}

TEST_CASE("PowerDecimatorBlock output count, DC gain and tone preservation for every plan", "[common][dsp][power_decim]")
{
    for (unsigned d = 2; d <= 8192; d *= 2)
    {
        INFO("decim = " << d);
        dsp::PowerDecimatorBlock<complex_t> dec(nullptr, d);

        // Enough input for 256 output samples; tone at 1/10th of the output bandwidth
        const size_t nout = 256;
        const double fout = 0.05; // cycles per output sample
        auto in = dsp_test::complexTone(nout * d, 2 * M_PI * fout / d);
        auto out = processChunked(dec, in, std::min<size_t>(in.size(), 65536));

        REQUIRE(out.size() == nout);
        // Skip the start-up transient of the cascade. The SDR++ derived plans allow ~0.5 dB of passband droop
        size_t skip = nout / 2;
        CHECK(dsp_test::rms(out, skip, nout) == Approx(1.0).margin(0.06));
        CHECK(dsp_test::estimateFreq(out, skip, nout) == Approx(2 * M_PI * fout).epsilon(1e-3));
    }
}

TEST_CASE("PowerDecimatorBlock attenuates tones beyond the output Nyquist", "[common][dsp][power_decim]")
{
    for (unsigned d : {2u, 8u, 64u, 1024u})
    {
        INFO("decim = " << d);
        dsp::PowerDecimatorBlock<complex_t> dec(nullptr, d);
        const size_t nout = 512;
        // 0.75 of the output samplerate would alias to -0.25
        auto in = dsp_test::complexTone(nout * d, 2 * M_PI * 0.75 / d);
        auto out = processChunked(dec, in, std::min<size_t>(in.size(), 65536));
        REQUIRE(out.size() == nout);
        CHECK(dsp_test::toDb(dsp_test::rms(out, nout / 2, nout)) < -40.0);
    }
}

TEST_CASE("PowerDecimatorBlock<float> as a streaming block", "[common][dsp][power_decim]")
{
    auto input = std::make_shared<dsp::stream<float>>();
    dsp::PowerDecimatorBlock<float> dec(input, 16);
    std::vector<float> in(160000, 2.0f);
    auto r = dsp_test::runBlock(dec, input, in, 8000);
    REQUIRE(r.completed);
    REQUIRE(r.output.size() == 10000);
    size_t mismatches = 0, first = 0;
    for (size_t i = 100; i < r.output.size(); i++)
        if (!(r.output[i] == Approx(2.0f).margin(0.01)) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}
