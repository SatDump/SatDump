#include "common/dsp/dsp_test_helpers.h"
#include "common/dsp/resamp/smart_resampler.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Approx;
using Catch::Matchers::WithinAbs;

namespace
{
    template <typename T>
    std::vector<T> processChunked(dsp::SmartResamplerBlock<T> &res, const std::vector<T> &in, size_t chunk, double ratio)
    {
        std::vector<T> out;
        // process() works in place in the output buffer, which must hold max(input, output) samples
        size_t osize = std::max<size_t>(chunk, (size_t)(chunk * ratio) + 16);
        dsp_test::VolkBuffer<T> ibuf(chunk), obuf(osize);
        for (size_t pos = 0; pos < in.size(); pos += chunk)
        {
            int n = std::min(chunk, in.size() - pos);
            memcpy((void *)ibuf.data(), &in[pos], n * sizeof(T));
            int outn = res.process(ibuf.data(), n, obuf.data());
            out.insert(out.end(), obuf.data(), obuf.data() + outn);
        }
        return out;
    }
} // namespace

TEST_CASE("SmartResamplerBlock 1:1 is a passthrough", "[common][dsp][smart_resampler]")
{
    dsp::SmartResamplerBlock<float> res(nullptr, 5, 5);
    std::vector<float> in(1000);
    for (size_t i = 0; i < in.size(); i++)
        in[i] = (float)i;
    CHECK(processChunked(res, in, 256, 1.0) == in);
}

TEST_CASE("SmartResamplerBlock output rate and tone frequency", "[common][dsp][smart_resampler]")
{
    struct Ratio
    {
        unsigned interp, decim;
    };
    // Covers: power decimator only, power decimator + resampler, resampler only (up and down)
    const Ratio ratios[] = {{1, 4}, {1, 64}, {3, 10}, {1, 6}, {2, 3}, {5, 2}, {48000, 2400000}};
    const double fs_in = 2.4e6;

    for (Ratio r : ratios)
    {
        INFO("interp = " << r.interp << ", decim = " << r.decim);
        double ratio = (double)r.interp / r.decim;
        double fs_out = fs_in * ratio;
        double tone = fs_out * 0.05; // Well inside the output passband

        dsp::SmartResamplerBlock<complex_t> res(nullptr, r.interp, r.decim);
        size_t nin = (size_t)(20000 / ratio);
        nin = std::max<size_t>(nin, 20000);
        auto out = processChunked(res, dsp_test::complexTone(nin, dsp::hz_to_rad(tone, fs_in)), 16384, ratio);

        CHECK_THAT((double)out.size(), WithinAbs(nin * ratio, 4.0));
        size_t skip = out.size() / 4;
        REQUIRE(out.size() > skip + 100);
        CHECK(dsp::rad_to_hz(dsp_test::estimateFreq(out, skip, out.size()), fs_out) == Approx(tone).epsilon(1e-3));
        // Allows for the ~0.5 dB passband droop of the power decimator plans
        CHECK(dsp_test::rms(out, skip, out.size()) == Approx(1.0).margin(0.06));
    }
}

TEST_CASE("SmartResamplerBlock as a streaming block", "[common][dsp][smart_resampler]")
{
    auto input = std::make_shared<dsp::stream<float>>();
    dsp::SmartResamplerBlock<float> res(input, 1, 10);
    auto r = dsp_test::runBlock(res, input, std::vector<float>(100000, 1.0f), 10000);
    REQUIRE(r.completed);
    CHECK_THAT((double)r.output.size(), WithinAbs(10000, 2.0));
    size_t mismatches = 0, first = 0;
    for (size_t i = 500; i < r.output.size(); i++)
        if (!(r.output[i] == Approx(1.0f).margin(0.01)) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}
