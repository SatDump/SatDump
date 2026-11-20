#include "common/dsp/demod/delay_one_imag.h"
#include "../dsp_test_helpers.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("DelayOneImagBlock delays Q by one sample across buffers", "[common][dsp][delay_one_imag]")
{
    std::vector<complex_t> in(10000);
    for (size_t i = 0; i < in.size(); i++)
        in[i] = complex_t((float)i, -(float)i - 0.5f);

    for (int chunk : {1, 7, 4096})
    {
        INFO("chunk = " << chunk);
        auto input = std::make_shared<dsp::stream<complex_t>>();
        dsp::DelayOneImagBlock blk(input);
        auto r = dsp_test::runBlock(blk, input, in, chunk);
        REQUIRE(r.completed);
        REQUIRE(r.output.size() == in.size());

        // I untouched, Q from the previous sample (0 before the first one)
        CHECK(r.output[0].real == 0.0f);
        CHECK(r.output[0].imag == 0.0f);
        for (size_t i = 1; i < in.size(); i++)
        {
            REQUIRE(r.output[i].real == in[i].real);
            REQUIRE(r.output[i].imag == in[i - 1].imag);
        }
    }
}
