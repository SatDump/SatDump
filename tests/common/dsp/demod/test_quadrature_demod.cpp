#include "common/dsp/demod/quadrature_demod.h"
#include "../dsp_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("QuadratureDemodBlock output is frequency divided by gain", "[common][dsp][quadrature_demod]")
{
    for (double w : {0.01, 0.3, -0.7, 2.5, -3.0})
    {
        for (float gain : {1.0f, 0.5f, 2.0f})
        {
            INFO("w = " << w << ", gain = " << gain);
            dsp::QuadratureDemodBlock qd(nullptr, gain);
            auto in = dsp_test::complexTone(1000, w, 0.3);
            std::vector<float> out(in.size());
            REQUIRE(qd.process(in.data(), in.size(), out.data()) == (int)in.size());
            // First sample measures from an initial phase of 0, which is the tone start phase
            for (size_t i = 0; i < out.size(); i++)
                REQUIRE(out[i] == Approx(i == 0 ? 0.0 : w / gain).margin(1e-4));
        }
    }
}

TEST_CASE("QuadratureDemodBlock keeps phase state between calls", "[common][dsp][quadrature_demod]")
{
    dsp::QuadratureDemodBlock qd(nullptr, 1.0f);
    auto in = dsp_test::complexTone(2000, 1.2, 1.0, 0.0);
    std::vector<float> out(1000);

    qd.process(in.data(), 1000, out.data());
    qd.process(in.data() + 1000, 1000, out.data());
    // No glitch at the boundary
    CHECK(out[0] == Approx(1.2).margin(1e-4));
    for (float v : out)
        REQUIRE(v == Approx(1.2).margin(1e-4));
}

TEST_CASE("QuadratureDemodBlock set_gain and amplitude independence", "[common][dsp][quadrature_demod]")
{
    dsp::QuadratureDemodBlock qd(nullptr, 1.0f);
    // Deviation normalization, as done by the FM demodulators: gain = hz_to_rad(deviation)
    qd.set_gain(dsp::hz_to_rad(5000, 48000));

    // Amplitude does not matter for an FM demodulator
    auto in = dsp_test::complexTone(500, dsp::hz_to_rad(2500, 48000), 1e-3);
    std::vector<float> out(in.size());
    qd.process(in.data(), in.size(), out.data());
    for (size_t i = 1; i < out.size(); i++)
        REQUIRE(out[i] == Approx(0.5).margin(1e-3));
}

TEST_CASE("QuadratureDemodBlock demodulates an FM signal", "[common][dsp][quadrature_demod]")
{
    // FM modulate a slow sine, demodulate, recover the sine
    const size_t n = 20000;
    const double dev = 0.4, wm = 2 * M_PI / 500;
    std::vector<complex_t> in(n);
    double phase = 0;
    for (size_t i = 0; i < n; i++)
    {
        in[i] = complex_t(cos(phase), sin(phase));
        phase += dev * sin(wm * (i + 1));
    }

    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::QuadratureDemodBlock qd(input, dev);
    auto r = dsp_test::runBlock(qd, input, in, 4096);
    REQUIRE(r.completed);
    REQUIRE(r.output.size() == n);
    for (size_t i = 1; i < n; i++)
        REQUIRE(r.output[i] == Approx(sin(wm * i)).margin(1e-3));
}
