#include "common/dsp/utils/real_to_complex.h"
#include "../dsp_test_helpers.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("RealToComplexBlock puts the input on the real axis", "[common][dsp][real_to_complex]")
{
    std::vector<float> in(9000);
    for (size_t i = 0; i < in.size(); i++)
        in[i] = (float)i * 0.25f - 1000.0f;

    auto input = std::make_shared<dsp::stream<float>>();
    dsp::RealToComplexBlock blk(input);
    auto r = dsp_test::runBlock(blk, input, in, 1234);
    REQUIRE(r.completed);
    REQUIRE(r.output.size() == in.size());
    for (size_t i = 0; i < in.size(); i++)
    {
        REQUIRE(r.output[i].real == in[i]);
        REQUIRE(r.output[i].imag == 0.0f);
    }
}
