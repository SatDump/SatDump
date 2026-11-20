#include "common/dsp/path/splitter_vfo.h"
#include "../dsp_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("VFOSplitterBlock output management", "[common][dsp][splitter_vfo]")
{
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::VFOSplitterBlock sp(input);
    CHECK(sp.get_vfo_output("a") == nullptr);

    // add_vfo starts a frequency shifter thread, del_vfo stops it
    sp.add_vfo("a", 1e6, 1e3);
    auto a = sp.get_vfo_output("a");
    REQUIRE(a != nullptr);
    sp.add_vfo("a", 1e6, 5e3); // Duplicate ignored
    CHECK(sp.get_vfo_output("a") == a);

    sp.del_vfo("a");
    CHECK(sp.get_vfo_output("a") == nullptr);
    sp.del_vfo("a");
    sp.set_vfo_enabled("nope", true);
}

TEST_CASE("VFOSplitterBlock feeds main and shifted VFO outputs", "[common][dsp][splitter_vfo]")
{
    const double fs = 2e6;
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::VFOSplitterBlock sp(input);
    sp.add_vfo("vfo", fs, -300e3);
    sp.add_vfo("off", fs, 50e3);
    sp.set_vfo_enabled("vfo", true);

    dsp_test::Collector<complex_t> main_col(sp.output_stream);
    dsp_test::Collector<complex_t> vfo_col(sp.get_vfo_output("vfo"));
    dsp_test::Collector<complex_t> off_col(sp.get_vfo_output("off"));

    auto in = dsp_test::complexTone(40000, dsp::hz_to_rad(250e3, fs));
    sp.start();
    bool fed = dsp_test::feedStream(input, in, 5000);
    // The VFO output goes through a second thread (the frequency shifter)
    bool got = fed && vfo_col.waitFor(in.size());
    sp.stop();
    sp.del_vfo("vfo");
    sp.del_vfo("off");
    auto main_out = main_col.finish();
    auto vfo = vfo_col.finish();
    auto off = off_col.finish();

    REQUIRE(fed);
    REQUIRE(got);
    REQUIRE(main_out.size() == in.size());
    for (size_t i = 0; i < in.size(); i++)
        REQUIRE(dsp_test::absDiff(main_out[i], in[i]) == 0.0f);

    REQUIRE(vfo.size() == in.size());
    CHECK(dsp::rad_to_hz(dsp_test::estimateFreq(vfo, 0, vfo.size()), fs) == Approx(-50e3).epsilon(1e-5));
    CHECK(off.empty());
}
