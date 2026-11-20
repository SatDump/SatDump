#include "dsp/block_simple.h"
#include "ndsp_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace satdump::ndsp;

namespace
{
    // Keeps every other sample, scaled. Records the sizes process() was called with.
    class DecimHalf : public BlockSimple<float, float>
    {
    public:
        std::vector<uint32_t> calls;
        DecimHalf() : BlockSimple<float, float>("decim_half_ff", {{"in", DSP_SAMPLE_TYPE_F32}}, {{"out", DSP_SAMPLE_TYPE_F32}}) {}

        uint32_t process(float *input, uint32_t nsamples, float *output) override
        {
            calls.push_back(nsamples);
            uint32_t n = 0;
            for (uint32_t i = 0; i < nsamples; i += 2)
                output[n++] = input[i] * 10;
            return n;
        }

        nlohmann::json get_cfg(std::string key) override { throw satdump_exception(key); }
        cfg_res_t set_cfg(std::string key, nlohmann::json) override { throw satdump_exception(key); }
    };

    // Duplicates every sample, needs an output twice as large
    class Dup : public BlockSimple<int16_t, int16_t>
    {
    public:
        Dup(float ratio) : BlockSimple<int16_t, int16_t>("dup_ss", {{"in", DSP_SAMPLE_TYPE_S16}}, {{"out", DSP_SAMPLE_TYPE_S16}}) { output_buffer_size_ratio = ratio; }

        uint32_t process(int16_t *input, uint32_t nsamples, int16_t *output) override
        {
            for (uint32_t i = 0; i < nsamples; i++)
                output[2 * i] = output[2 * i + 1] = input[i];
            return nsamples * 2;
        }

        nlohmann::json get_cfg(std::string key) override { throw satdump_exception(key); }
        cfg_res_t set_cfg(std::string key, nlohmann::json) override { throw satdump_exception(key); }
    };

    // Drops everything
    class Drop : public BlockSimple<float, float>
    {
    public:
        Drop() : BlockSimple<float, float>("drop_ff", {{"in", DSP_SAMPLE_TYPE_F32}}, {{"out", DSP_SAMPLE_TYPE_F32}}) {}
        uint32_t process(float *, uint32_t, float *) override { return 0; }
        nlohmann::json get_cfg(std::string key) override { throw satdump_exception(key); }
        cfg_res_t set_cfg(std::string key, nlohmann::json) override { throw satdump_exception(key); }
    };
} // namespace

TEST_CASE("ndsp BlockSimple calls process once per input buffer", "[dsp][block_simple]")
{
    auto b = std::make_shared<DecimHalf>();
    std::vector<float> in = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto r = ndsp_test::runSimple<float, float>(b, in, 4);
    REQUIRE(r.completed);
    // Each buffer is processed on its own : [1 2 3 4] [5 6 7 8] [9 10]
    CHECK(b->calls == std::vector<uint32_t>{4, 4, 2});
    CHECK(r.out() == std::vector<float>{10, 30, 50, 70, 90});
    CHECK(r.outputs[0].sizes == std::vector<uint32_t>{2, 2, 1});
    CHECK(r.outputs[0].propagating_terminators == 1);
}

TEST_CASE("ndsp BlockSimple output buffers are sized from the input max_size", "[dsp][block_simple]")
{
    auto b = std::make_shared<Dup>(2.0f);
    std::vector<int16_t> in = {1, -2, 3, 32767, -32768};
    auto r = ndsp_test::runSimple<int16_t, int16_t>(b, in, 3);
    REQUIRE(r.completed);
    CHECK(r.out() == std::vector<int16_t>{1, 1, -2, -2, 3, 3, 32767, 32767, -32768, -32768});
    // Input buffers have max_size 3 (full) and 2 (last chunk)
    CHECK(r.outputs[0].max_sizes == std::vector<uint32_t>{6, 4});
}

TEST_CASE("ndsp BlockSimple rounds the output buffer size up", "[dsp][block_simple]")
{
    // 3 * 1.5 = 4.5 -> 5 samples allocated
    class Ratio : public BlockSimple<float, float>
    {
    public:
        Ratio() : BlockSimple<float, float>("ratio_ff", {{"in", DSP_SAMPLE_TYPE_F32}}, {{"out", DSP_SAMPLE_TYPE_F32}}) { output_buffer_size_ratio = 1.5; }
        uint32_t process(float *input, uint32_t nsamples, float *output) override
        {
            memcpy(output, input, nsamples * sizeof(float));
            return nsamples;
        }
        nlohmann::json get_cfg(std::string key) override { throw satdump_exception(key); }
        cfg_res_t set_cfg(std::string key, nlohmann::json) override { throw satdump_exception(key); }
    };
    auto b = std::make_shared<Ratio>();
    auto r = ndsp_test::runSimple<float, float>(b, {1, 2, 3}, 3);
    REQUIRE(r.completed);
    CHECK(r.outputs[0].max_sizes == std::vector<uint32_t>{5});
}

TEST_CASE("ndsp BlockSimple sends no buffer when process returns 0", "[dsp][block_simple]")
{
    auto b = std::make_shared<Drop>();
    auto r = ndsp_test::runSimple<float, float>(b, ndsp_test::randomFloats(100), 10);
    REQUIRE(r.completed);
    CHECK(r.out().empty());
    CHECK(r.outputs[0].sizes.empty());
    CHECK(r.outputs[0].terminators == 1);
}

TEST_CASE("ndsp BlockSimple forwards only propagating terminators", "[dsp][block_simple]")
{
    auto b = std::make_shared<DecimHalf>();
    auto r = ndsp_test::runSimple<float, float>(b, {1, 2}, 2, 2);
    REQUIRE(r.completed);
    CHECK(r.out() == std::vector<float>{10});
    CHECK(r.outputs[0].terminators == 0);
}

TEST_CASE("ndsp BlockSimple handles an empty input buffer", "[dsp][block_simple]")
{
    auto b = std::make_shared<DecimHalf>();
    auto in = std::make_shared<DSPStream>(4);
    b->set_input({"x", DSP_SAMPLE_TYPE_F32, in}, 0);
    ndsp_test::Collector<float> col(b->get_output(0, 4).fifo);
    b->start();
    ndsp_test::pushSamples<float>(*in, nullptr, 0);
    ndsp_test::pushSamples<float>(*in, std::vector<float>{4, 5});
    ndsp_test::pushTerminator(*in);
    REQUIRE(ndsp_test::stopBlock(b));
    REQUIRE(col.waitForTerminator());
    col.finish();
    CHECK(b->calls == std::vector<uint32_t>{0, 2});
    CHECK(col.data() == std::vector<float>{40});
}

TEST_CASE("ndsp BlockSimple chains blocks through link", "[dsp][block_simple]")
{
    auto a = std::make_shared<DecimHalf>();
    auto b = std::make_shared<DecimHalf>();
    b->link(a.get(), 0, 0, 4);

    auto in = std::make_shared<DSPStream>(4);
    a->set_input({"x", DSP_SAMPLE_TYPE_F32, in}, 0);
    ndsp_test::Collector<float> col(b->get_output(0, 4).fifo);
    a->start();
    b->start();

    std::vector<float> data(64);
    for (int i = 0; i < 64; i++)
        data[i] = i;
    ndsp_test::Feeder<float> feed(in, data, 16);
    REQUIRE(feed.wait());
    REQUIRE(col.waitForTerminator());
    REQUIRE(ndsp_test::stopBlock(a));
    REQUIRE(ndsp_test::stopBlock(b));
    col.finish();

    // Each stage keeps even indexes of its own (16 then 8 sample) buffers and multiplies by 10
    auto d = col.data();
    REQUIRE(d.size() == 16);
    for (int i = 0; i < 16; i++)
        CHECK(d[i] == 100.0f * (4 * i));
}
