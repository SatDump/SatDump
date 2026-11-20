#include "common/dsp/pll/costas_loop.h"
#include "../dsp_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

namespace
{
    /*
    Angle of constellation point 0 the loop locks to. QPSK sits on the diagonals.
    The order 8 detector (same as GNU Radio's) has its zeros at pi/8 + k pi/4, so it
    locks 8PSK with a pi/8 rotation compared to dsp::constellation_t's PSK8 points.
    */
    double lockOffset(int order) { return order == 4 ? M_PI / 4 : (order == 8 ? M_PI / 8 : 0.0); }

    // order-PSK symbols at one sample per symbol, rotated by a carrier offset
    std::vector<complex_t> makePsk(int order, size_t n, double freq, double phase0, std::vector<int> &sym, uint32_t seed)
    {
        dsp_test::Lcg rng(seed);
        std::vector<complex_t> v(n);
        sym.resize(n);
        for (size_t i = 0; i < n; i++)
        {
            sym[i] = rng.next() % order;
            double a = 2 * M_PI * sym[i] / order + lockOffset(order);
            double p = a + phase0 + freq * i;
            v[i] = complex_t(cos(p), sin(p));
        }
        return v;
    }

    // Fraction of symbols recovered, for the best of the order-fold phase ambiguities
    double symbolMatch(const std::vector<complex_t> &out, const std::vector<int> &sym, int order, size_t start)
    {
        double best = 0;
        for (int rot = 0; rot < order; rot++)
        {
            size_t ok = 0;
            for (size_t i = start; i < out.size(); i++)
            {
                double a = atan2(out[i].imag, out[i].real) - lockOffset(order);
                int k = ((int)std::lround(a / (2 * M_PI / order)) % order + order) % order;
                ok += k == (sym[i] + rot) % order;
            }
            best = std::max(best, (double)ok / (out.size() - start));
        }
        return best;
    }

    // Mean absolute angle between the output and the nearest ideal constellation point
    double residualPhase(const std::vector<complex_t> &out, int order, size_t start)
    {
        double step = 2 * M_PI / order, acc = 0;
        for (size_t i = start; i < out.size(); i++)
        {
            double a = atan2(out[i].imag, out[i].real) - lockOffset(order);
            acc += fabs(remainder(a, step));
        }
        return acc / (out.size() - start);
    }

    struct CostasRun
    {
        std::vector<complex_t> out;
        float freq;
    };

    CostasRun runCostas(const std::vector<complex_t> &in, float bw, unsigned order, float limit = 1.0f)
    {
        auto input = std::make_shared<dsp::stream<complex_t>>();
        dsp::CostasLoopBlock costas(input, bw, order, limit);
        auto r = dsp_test::runBlock(costas, input, in, 4096);
        REQUIRE(r.completed);
        REQUIRE(r.output.size() == in.size());
        return {r.output, costas.getFreq()};
    }
}

TEST_CASE("CostasLoopBlock order 2 locks onto BPSK with a frequency offset", "[common][dsp][costas]")
{
    for (double f : {0.01, -0.005, 0.0})
    {
        INFO("freq = " << f);
        std::vector<int> sym;
        auto in = makePsk(2, 30000, f, 0.7, sym, 60);
        auto r = runCostas(in, 0.01f, 2);

        CHECK(r.freq == Approx(f).margin(5e-4));
        size_t start = in.size() / 2;
        CHECK(residualPhase(r.out, 2, start) < 0.05);
        CHECK(symbolMatch(r.out, sym, 2, start) > 0.999);
    }
}

TEST_CASE("CostasLoopBlock order 4 locks onto QPSK with a frequency offset", "[common][dsp][costas]")
{
    std::vector<int> sym;
    auto in = makePsk(4, 30000, -0.008, 2.0, sym, 61);
    auto r = runCostas(in, 0.01f, 4);

    CHECK(r.freq == Approx(-0.008).margin(5e-4));
    size_t start = in.size() / 2;
    CHECK(residualPhase(r.out, 4, start) < 0.05);
    CHECK(symbolMatch(r.out, sym, 4, start) > 0.999);
}

TEST_CASE("CostasLoopBlock order 8 locks onto 8PSK with a frequency offset", "[common][dsp][costas]")
{
    std::vector<int> sym;
    auto in = makePsk(8, 40000, 0.004, -1.0, sym, 62);
    auto r = runCostas(in, 0.005f, 8);

    CHECK(r.freq == Approx(0.004).margin(5e-4));
    size_t start = in.size() / 2;
    CHECK(residualPhase(r.out, 8, start) < 0.05);
    CHECK(symbolMatch(r.out, sym, 8, start) > 0.999);
}

TEST_CASE("CostasLoopBlock order 8 settles pi/8 away from axis-aligned 8PSK", "[common][dsp][costas]")
{
    // Points at k pi/4 (dsp::constellation_t PSK8 layout) end up rotated by pi/8 at the output
    std::vector<int> sym;
    auto in = makePsk(8, 40000, 0.0, 0.0, sym, 66);
    for (auto &v : in)
        v = v * complex_t(cosf(-M_PI / 8), sinf(-M_PI / 8));
    auto r = runCostas(in, 0.005f, 8);
    CHECK(residualPhase(r.out, 8, in.size() / 2) < 0.05);
}

TEST_CASE("CostasLoopBlock clamps the frequency to the limit", "[common][dsp][costas]")
{
    std::vector<int> sym;
    auto in = makePsk(2, 20000, 0.02, 0.0, sym, 63);
    auto r = runCostas(in, 0.02f, 2, 0.005f);
    CHECK(r.freq == Approx(0.005f));

    auto rn = runCostas(makePsk(2, 20000, -0.02, 0.0, sym, 64), 0.02f, 2, 0.005f);
    CHECK(rn.freq == Approx(-0.005f));
}

TEST_CASE("CostasLoopBlock preserves amplitude", "[common][dsp][costas]")
{
    std::vector<int> sym;
    auto in = makePsk(2, 5000, 0.003, 0.0, sym, 65);
    for (auto &v : in)
        v *= 0.25f;
    auto r = runCostas(in, 0.01f, 2);
    for (size_t i = 0; i < in.size(); i++)
        REQUIRE(r.out[i].norm() == Approx(0.25f).epsilon(1e-4));
}
