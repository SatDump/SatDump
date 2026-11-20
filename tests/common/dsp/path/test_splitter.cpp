#include "common/dsp/dsp_test_helpers.h"
#include "common/dsp/path/splitter.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

namespace
{
    std::vector<complex_t> ramp(size_t n)
    {
        std::vector<complex_t> v(n);
        for (size_t i = 0; i < n; i++)
            v[i] = complex_t((float)i, -(float)i);
        return v;
    }
} // namespace

TEST_CASE("SplitterBlock output management", "[common][dsp][splitter]")
{
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::SplitterBlock sp(input);

    CHECK(sp.get_output("a") == nullptr);
    CHECK(sp.get_vfo_output("v") == nullptr);

    sp.add_output("a");
    auto a = sp.get_output("a");
    REQUIRE(a != nullptr);
    CHECK(a != sp.output_stream);

    // Adding twice keeps the same stream
    sp.add_output("a");
    CHECK(sp.get_output("a") == a);

    // Reset gives a fresh stream
    sp.reset_output("a");
    CHECK(sp.get_output("a") != a);
    CHECK(sp.get_output("a") != nullptr);

    sp.del_output("a");
    CHECK(sp.get_output("a") == nullptr);

    sp.add_vfo("v", 1e6, 1000);
    auto v = sp.get_vfo_output("v");
    REQUIRE(v != nullptr);
    sp.reset_vfo("v");
    CHECK(sp.get_vfo_output("v") != v);
    sp.del_vfo("v");
    CHECK(sp.get_vfo_output("v") == nullptr);

    // Operations on unknown ids are no-ops
    sp.del_output("nope");
    sp.set_enabled("nope", true);
    sp.reset_output("nope");
    sp.del_vfo("nope");
    sp.set_vfo_enabled("nope", true);
    sp.reset_vfo("nope");
}

TEST_CASE("SplitterBlock copies the input to main and enabled outputs", "[common][dsp][splitter]")
{
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::SplitterBlock sp(input);
    sp.add_output("copy");
    sp.add_output("disabled");
    sp.set_enabled("copy", true);

    dsp_test::Collector<complex_t> main_col(sp.output_stream);
    dsp_test::Collector<complex_t> copy_col(sp.get_output("copy"));
    dsp_test::Collector<complex_t> dis_col(sp.get_output("disabled"));

    auto in = ramp(20000);
    sp.start();
    bool fed = dsp_test::feedStream(input, in, 3000);
    sp.stop();
    auto main_out = main_col.finish();
    auto copy_out = copy_col.finish();
    auto dis_out = dis_col.finish();

    REQUIRE(fed);
    REQUIRE(main_out.size() == in.size());
    REQUIRE(copy_out.size() == in.size());
    CHECK(dis_out.empty());
    size_t mismatches = 0, first = 0;
    for (size_t i = 0; i < in.size(); i++)
        if ((!(main_out[i].real == in[i].real) || !(copy_out[i].imag == in[i].imag)) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}

TEST_CASE("SplitterBlock VFO outputs are frequency shifted", "[common][dsp][splitter]")
{
    const double fs = 1e6;
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::SplitterBlock sp(input);
    sp.set_main_enabled(false);
    sp.add_vfo("up", fs, 100e3);
    sp.add_vfo("zero", fs, 0);
    sp.set_vfo_enabled("up", true);
    sp.set_vfo_enabled("zero", true);

    dsp_test::Collector<complex_t> main_col(sp.output_stream);
    dsp_test::Collector<complex_t> up_col(sp.get_vfo_output("up"));
    dsp_test::Collector<complex_t> zero_col(sp.get_vfo_output("zero"));

    auto in = dsp_test::complexTone(30000, dsp::hz_to_rad(-20e3, fs));
    sp.start();
    bool fed = dsp_test::feedStream(input, in, 4096);
    sp.stop();
    auto main_out = main_col.finish();
    auto up = up_col.finish();
    auto zero = zero_col.finish();

    REQUIRE(fed);
    // Main disabled
    CHECK(main_out.empty());

    // -20 kHz shifted by +100 kHz
    REQUIRE(up.size() == in.size());
    CHECK(dsp::rad_to_hz(dsp_test::estimateFreq(up, 0, up.size()), fs) == Approx(80e3).epsilon(1e-5));

    // A VFO at 0 Hz is a plain copy
    REQUIRE(zero.size() == in.size());
    size_t mismatches = 0, first = 0;
    for (size_t i = 0; i < in.size(); i++)
        if (!(dsp_test::absDiff(zero[i], in[i]) == 0.0f) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}
