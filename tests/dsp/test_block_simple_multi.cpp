#include "dsp/block_simple_multi.h"
#include "ndsp_test_helpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace satdump::ndsp;

namespace
{
    // 2 inputs -> 2 outputs : sum and difference. Output 1 only keeps the first half.
    class SumDiff : public BlockSimpleMulti<float, float, 2, 2>
    {
    public:
        SumDiff()
            : BlockSimpleMulti<float, float, 2, 2>("sumdiff_ff", {{"a", DSP_SAMPLE_TYPE_F32}, {"b", DSP_SAMPLE_TYPE_F32}},
                                                   {{"sum", DSP_SAMPLE_TYPE_F32}, {"diff", DSP_SAMPLE_TYPE_F32}})
        {
            output_buffer_size_ratio[1] = 0.5;
        }

        void process(float **input, uint32_t *nsamples, float **output, uint32_t *nsamples_out) override
        {
            uint32_t n = std::min(nsamples[0], nsamples[1]);
            for (uint32_t i = 0; i < n; i++)
                output[0][i] = input[0][i] + input[1][i];
            for (uint32_t i = 0; i < n / 2; i++)
                output[1][i] = input[0][i] - input[1][i];
            nsamples_out[0] = n;
            nsamples_out[1] = n / 2;
        }

        nlohmann::json get_cfg(std::string key) override { throw satdump_exception(key); }
        cfg_res_t set_cfg(std::string key, nlohmann::json) override { throw satdump_exception(key); }
    };
} // namespace

TEST_CASE("ndsp BlockSimpleMulti processes one buffer of each input at a time", "[dsp][block_simple_multi]")
{
    auto b = std::make_shared<SumDiff>();
    std::vector<float> a = {1, 2, 3, 4, 5, 6};
    std::vector<float> c = {10, 20, 30, 40, 50, 60};
    auto r = ndsp_test::runBlock<float, float>(b, {a, c}, 4);
    REQUIRE(r.completed);
    REQUIRE(r.outputs.size() == 2);
    CHECK(r.outputs[0].data == std::vector<float>{11, 22, 33, 44, 55, 66});
    // Half of each buffer : [4] -> 2, [2] -> 1
    CHECK(r.outputs[1].data == std::vector<float>{-9, -18, -45});
    CHECK(r.outputs[0].sizes == std::vector<uint32_t>{4, 2});
    CHECK(r.outputs[1].sizes == std::vector<uint32_t>{2, 1});
    // Output 1 buffers are allocated with the ratio applied to the input max_size
    CHECK(r.outputs[0].max_sizes == std::vector<uint32_t>{4, 2});
    CHECK(r.outputs[1].max_sizes == std::vector<uint32_t>{2, 1});
    // Terminator goes to every output
    CHECK(r.outputs[0].propagating_terminators == 1);
    CHECK(r.outputs[1].propagating_terminators == 1);
}

TEST_CASE("ndsp BlockSimpleMulti non-propagating terminator stops silently", "[dsp][block_simple_multi]")
{
    auto b = std::make_shared<SumDiff>();
    auto r = ndsp_test::runBlock<float, float>(b, {{1, 2}, {3, 4}}, 8, 2);
    REQUIRE(r.completed);
    CHECK(r.outputs[0].data == std::vector<float>{4, 6});
    CHECK(r.outputs[0].terminators == 0);
    CHECK(r.outputs[1].terminators == 0);
}

TEST_CASE("ndsp BlockSimpleMulti uses the largest input buffer for allocation", "[dsp][block_simple_multi]")
{
    auto b = std::make_shared<SumDiff>();
    auto ia = std::make_shared<DSPStream>(4);
    auto ib = std::make_shared<DSPStream>(4);
    b->set_input({"a", DSP_SAMPLE_TYPE_F32, ia}, 0);
    b->set_input({"b", DSP_SAMPLE_TYPE_F32, ib}, 1);
    ndsp_test::Collector<float> c0(b->get_output(0, 4).fifo);
    ndsp_test::Collector<float> c1(b->get_output(1, 4).fifo);
    b->start();

    ndsp_test::pushSamples(*ia, std::vector<float>{1, 1});
    ndsp_test::pushSamples(*ib, std::vector<float>{2, 2, 2, 2, 2, 2, 2, 2});
    ndsp_test::pushTerminator(*ia);
    ndsp_test::pushTerminator(*ib);

    REQUIRE(ndsp_test::stopBlock(b));
    REQUIRE(c0.waitForTerminator());
    REQUIRE(c1.waitForTerminator());
    c0.finish();
    c1.finish();
    CHECK(c0.data() == std::vector<float>{3, 3});
    CHECK(c0.maxSizes() == std::vector<uint32_t>{8});
    CHECK(c1.maxSizes() == std::vector<uint32_t>{4});
}
