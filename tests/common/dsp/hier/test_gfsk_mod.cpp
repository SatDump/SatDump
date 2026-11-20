#include "common/dsp/hier/gfsk_mod.h"
#include "../dsp_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

namespace
{
    // Instantaneous frequency (rad/sample) between consecutive samples
    float instFreq(const std::vector<complex_t> &v, size_t i)
    {
        std::complex<float> a(v[i].real, v[i].imag), b(v[i - 1].real, v[i - 1].imag);
        return std::arg(a * std::conj(b));
    }

    std::vector<complex_t> runGfsk(const std::vector<float> &in, float sensitivity, float bt)
    {
        auto input = std::make_shared<dsp::stream<float>>();
        dsp::GFSKMod mod(input, sensitivity, bt);
        // Two internal blocks: wait for the final stage to have produced everything
        auto r = dsp_test::runBlock(mod, input, in, 1000, 2 * in.size());
        REQUIRE(r.completed);
        return r.output;
    }
}

TEST_CASE("GFSKMod produces a constant envelope at 2 samples per symbol", "[common][dsp][gfsk_mod]")
{
    dsp_test::Lcg rng(120);
    std::vector<float> sym(5000);
    for (auto &s : sym)
        s = rng.sign();

    auto out = runGfsk(sym, M_PI / 4, 0.5f);
    REQUIRE(out.size() == 2 * sym.size());
    for (const auto &v : out)
        REQUIRE(hypotf(v.real, v.imag) == Approx(1.0f).epsilon(1e-5));
}

TEST_CASE("GFSKMod frequency follows the symbols", "[common][dsp][gfsk_mod]")
{
    // Long runs of identical symbols: the frequency settles at +-sensitivity per sample
    const float sens = M_PI / 4; // GMSK
    std::vector<float> sym;
    for (int run = 0; run < 20; run++)
        for (int i = 0; i < 40; i++)
            sym.push_back(run % 2 ? -1.0f : 1.0f);

    auto out = runGfsk(sym, sens, 0.5f);
    REQUIRE(out.size() == 2 * sym.size());

    // Middle of each run, after the Gaussian filter and resampler delay settled
    for (int run = 1; run < 20; run++)
    {
        INFO("run " << run);
        size_t mid = 2 * (run * 40 + 30);
        float expected = run % 2 ? -sens : sens;
        CHECK(instFreq(out, mid) == Approx(expected).margin(0.01));
    }
}
