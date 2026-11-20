#include "common/dsp/dsp_test_helpers.h"
#include "common/dsp/resamp/rational_resampler.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <complex>

using Catch::Approx;
using Catch::Matchers::WithinAbs;

namespace
{
    template <typename T, typename R>
    std::vector<T> processChunked(R &res, const std::vector<T> &in, size_t chunk, double ratio)
    {
        std::vector<T> out;
        dsp_test::VolkBuffer<T> ibuf(chunk), obuf((size_t)(chunk * ratio) + 16);
        for (size_t pos = 0; pos < in.size(); pos += chunk)
        {
            int n = std::min(chunk, in.size() - pos);
            memcpy((void *)ibuf.data(), &in[pos], n * sizeof(T));
            int outn = res.process(ibuf.data(), n, obuf.data());
            REQUIRE(outn <= (int)obuf.size());
            out.insert(out.end(), obuf.data(), obuf.data() + outn);
        }
        return out;
    }

    // RMS of what is left after removing the best-fitting tone at freq: measures interpolation distortion
    double toneResidual(const std::vector<complex_t> &y, size_t start, size_t end, double freq)
    {
        std::complex<double> a = 0;
        for (size_t n = start; n < end; n++)
            a += std::complex<double>(y[n].real, y[n].imag) * std::polar(1.0, -freq * n);
        a /= (double)(end - start);

        double err = 0;
        for (size_t n = start; n < end; n++)
            err += std::norm(std::complex<double>(y[n].real, y[n].imag) - a * std::polar(1.0, freq * n));
        return sqrt(err / (end - start)) / std::abs(a);
    }
} // namespace

TEST_CASE("RationalResamplerBlock output count follows interp / decim", "[common][dsp][rational_resampler]")
{
    struct Ratio
    {
        unsigned interp, decim;
    };
    for (Ratio r : {Ratio{3, 2}, Ratio{2, 3}, Ratio{1, 4}, Ratio{5, 1}, Ratio{7, 11}})
    {
        INFO("interp = " << r.interp << ", decim = " << r.decim);
        dsp::RationalResamplerBlock<complex_t> res(nullptr, r.interp, r.decim);
        std::vector<complex_t> in(44000, complex_t(1, 0));
        double ratio = (double)r.interp / r.decim;
        auto out = processChunked(res, in, 4000, ratio);
        CHECK_THAT((double)out.size(), WithinAbs(in.size() * ratio, 2.0));
    }
}

TEST_CASE("RationalResamplerBlock has unity DC gain", "[common][dsp][rational_resampler]")
{
    dsp::RationalResamplerBlock<float> res(nullptr, 3, 2);
    std::vector<float> in(30000, 0.5f);
    auto out = processChunked(res, in, 3000, 1.5);
    REQUIRE(out.size() > 1000);
    size_t mismatches = 0, first = 0;
    for (size_t i = 500; i < out.size(); i++)
        if (!(out[i] == Approx(0.5f).margin(0.005)) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}

TEST_CASE("RationalResamplerBlock preserves tone frequency in Hz", "[common][dsp][rational_resampler]")
{
    struct Ratio
    {
        unsigned interp, decim;
    };
    const double fs_in = 48000, tone = 3000;
    for (Ratio r : {Ratio{3, 2}, Ratio{2, 3}, Ratio{4, 1}, Ratio{1, 3}})
    {
        INFO("interp = " << r.interp << ", decim = " << r.decim);
        double fs_out = fs_in * r.interp / r.decim;
        dsp::RationalResamplerBlock<complex_t> res(nullptr, r.interp, r.decim);
        auto out = processChunked(res, dsp_test::complexTone(48000, dsp::hz_to_rad(tone, fs_in)), 4096, (double)r.interp / r.decim);

        size_t skip = 2000;
        REQUIRE(out.size() > skip + 1000);
        double f = dsp_test::estimateFreq(out, skip, out.size());
        CHECK(dsp::rad_to_hz(f, fs_out) == Approx(tone).epsilon(1e-4));
        CHECK(dsp_test::rms(out, skip, out.size()) == Approx(1.0).margin(0.01));
        // The output is a clean tone: the polyphase arms are used in the right order
        CHECK(dsp_test::toDb(toneResidual(out, skip, out.size(), f)) < -50.0);
    }
}

TEST_CASE("RationalResamplerBlock reduces the ratio by its GCD", "[common][dsp][rational_resampler]")
{
    auto in = dsp_test::complexTone(20000, 0.05);
    dsp::RationalResamplerBlock<complex_t> a(nullptr, 3, 2);
    dsp::RationalResamplerBlock<complex_t> b(nullptr, 30, 20);
    auto oa = processChunked(a, in, 2000, 1.5);
    auto ob = processChunked(b, in, 2000, 1.5);
    REQUIRE(oa.size() == ob.size());
    size_t mismatches = 0, first = 0;
    for (size_t i = 0; i < oa.size(); i++)
        if (!(dsp_test::absDiff(oa[i], ob[i]) < 1e-6f) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}

TEST_CASE("RationalResamplerBlock with custom taps and set_ratio", "[common][dsp][rational_resampler]")
{
    // A single unity tap with 1/1 is a pure passthrough
    dsp::RationalResamplerBlock<float> res(nullptr, 1, 1, {1.0f});
    std::vector<float> in(1000);
    for (size_t i = 0; i < in.size(); i++)
        in[i] = (float)i;
    auto out = processChunked(res, in, 300, 1.0);
    REQUIRE(out.size() == in.size());
    size_t pass_mismatches = 0, pass_first = 0;
    for (size_t i = 0; i < in.size(); i++)
        if (!(out[i] == in[i]) && pass_mismatches++ == 0)
            pass_first = i;
    INFO("first passthrough mismatch at sample " << pass_first);
    REQUIRE(pass_mismatches == 0);

    // Zero-order hold interpolation by 2 with custom taps {1, 1}
    dsp::RationalResamplerBlock<float> zoh(nullptr, 2, 1, {1.0f, 1.0f});
    auto held = processChunked(zoh, in, 300, 2.0);
    REQUIRE(held.size() == 2 * in.size());
    size_t hold_mismatches = 0, hold_first = 0;
    for (size_t i = 0; i < in.size(); i++)
        if ((!(held[2 * i] == in[i]) || !(held[2 * i + 1] == in[i])) && hold_mismatches++ == 0)
            hold_first = i;
    INFO("first hold mismatch at input sample " << hold_first);
    REQUIRE(hold_mismatches == 0);

    // Changing the ratio on the fly changes the output rate
    res.set_ratio(1, 2);
    auto halved = processChunked(res, std::vector<float>(10000, 1.0f), 1000, 0.5);
    CHECK_THAT((double)halved.size(), WithinAbs(5000, 2.0));
}

TEST_CASE("RationalResamplerBlock as a streaming block", "[common][dsp][rational_resampler]")
{
    const double fs_in = 10000;
    auto in = dsp_test::complexTone(50000, dsp::hz_to_rad(400, fs_in));
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::RationalResamplerBlock<complex_t> res(input, 5, 2);
    auto r = dsp_test::runBlock(res, input, in, 4000);
    REQUIRE(r.completed);
    CHECK_THAT((double)r.output.size(), WithinAbs(125000, 2.0));
    double f = dsp_test::estimateFreq(r.output, 1000, r.output.size());
    CHECK(dsp::rad_to_hz(f, fs_in * 2.5) == Approx(400).epsilon(1e-4));
}
