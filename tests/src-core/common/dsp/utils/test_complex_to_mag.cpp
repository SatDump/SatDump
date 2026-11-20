#include "common/dsp/dsp_test_helpers.h"
#include "common/dsp/utils/complex_to_mag.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("ComplexToMagBlock outputs |z|", "[common][dsp][complex_to_mag]")
{
    dsp_test::Lcg rng(90);
    std::vector<complex_t> in(10007);
    for (auto &v : in)
        v = complex_t(rng.uniform() * 20 - 10, rng.uniform() * 20 - 10);
    in[0] = complex_t(3, 4);
    in[1] = complex_t(0, 0);
    in[2] = complex_t(-5, 12);

    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::ComplexToMagBlock blk(input);
    auto r = dsp_test::runBlock(blk, input, in, 1001);
    REQUIRE(r.completed);
    REQUIRE(r.output.size() == in.size());

    CHECK(r.output[0] == Approx(5.0f));
    CHECK(r.output[1] == 0.0f);
    CHECK(r.output[2] == Approx(13.0f));
    size_t mismatches = 0, first = 0;
    for (size_t i = 0; i < in.size(); i++)
        if (!(r.output[i] == Approx(std::hypot(in[i].real, in[i].imag)).epsilon(1e-5)) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}

TEST_CASE("ComplexToMagBlock demodulates AM", "[common][dsp][complex_to_mag]")
{
    // Carrier with 50 % AM by a slow tone: envelope = 1 + 0.5 cos(wm n)
    const size_t n = 8000;
    std::vector<complex_t> in(n);
    for (size_t i = 0; i < n; i++)
    {
        double env = 1 + 0.5 * cos(0.01 * i);
        in[i] = complex_t(env * cos(0.7 * i), env * sin(0.7 * i));
    }
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::ComplexToMagBlock blk(input);
    auto r = dsp_test::runBlock(blk, input, in, 2048);
    REQUIRE(r.completed);
    size_t mismatches = 0, first = 0;
    for (size_t i = 0; i < n; i++)
        if (!(r.output[i] == Approx(1 + 0.5 * cos(0.01 * i)).margin(1e-5)) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}
