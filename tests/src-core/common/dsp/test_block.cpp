#include "common/dsp/block.h"
#include "common/dsp/dsp_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using Catch::Approx;

namespace
{
    // Minimal block following the same conventions as the real ones
    class ScaleTestBlock : public dsp::Block<float, float>
    {
    public:
        int work_calls = 0;

        ScaleTestBlock(std::shared_ptr<dsp::stream<float>> input, float k) : Block(input), k(k) {}

    private:
        float k;

        void work()
        {
            int nsamples = input_stream->read();
            work_calls++;
            if (nsamples <= 0)
            {
                input_stream->flush();
                return;
            }

            for (int i = 0; i < nsamples; i++)
                output_stream->writeBuf[i] = input_stream->readBuf[i] * k;

            input_stream->flush();
            output_stream->swap(nsamples);
        }
    };
} // namespace

TEST_CASE("branchless_clip and branched_clip agree and saturate", "[common][dsp][block]")
{
    for (float x = -5.0f; x <= 5.0f; x += 0.25f)
    {
        float expected = x < -2.0f ? -2.0f : (x > 2.0f ? 2.0f : x);
        CHECK(dsp::branched_clip(x, 2.0f) == expected);
        CHECK(dsp::branchless_clip(x, 2.0f) == Approx(expected).margin(1e-6));
        CHECK(BRANCHLESS_CLIP(x, 2.0f) == Approx(expected).margin(1e-6));
    }

    CHECK(dsp::branched_clip(1e9f, 1.0f) == 1.0f);
    CHECK(dsp::branched_clip(-1e9f, 1.0f) == -1.0f);
}

TEST_CASE("hz_to_rad and rad_to_hz are inverses", "[common][dsp][block]")
{
    CHECK(dsp::hz_to_rad(0.0, 48000) == 0.0);
    CHECK(dsp::hz_to_rad(12000, 48000) == Approx(M_PI / 2));
    CHECK(dsp::hz_to_rad(24000, 48000) == Approx(M_PI));
    CHECK(dsp::hz_to_rad(-6000, 48000) == Approx(-M_PI / 4));
    CHECK(dsp::rad_to_hz(M_PI, 2e6) == Approx(1e6));

    for (double f : {-1e5, -1.0, 0.5, 1234.5, 3e6})
        CHECK(dsp::rad_to_hz(dsp::hz_to_rad(f, 6e6), 6e6) == Approx(f));
}

TEST_CASE("Block lifecycle: start, process, stop and join", "[common][dsp][block]")
{
    auto input = std::make_shared<dsp::stream<float>>();
    ScaleTestBlock blk(input, 2.0f);

    std::vector<float> data(10000);
    for (size_t i = 0; i < data.size(); i++)
        data[i] = (float)i;

    auto r = dsp_test::runBlock(blk, input, data, 999);
    REQUIRE(r.completed);
    REQUIRE(r.output.size() == data.size());
    size_t mismatches = 0, first = 0;
    for (size_t i = 0; i < data.size(); i++)
        if (!(r.output[i] == 2.0f * data[i]) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}

TEST_CASE("Block stop without start is safe", "[common][dsp][block]")
{
    auto input = std::make_shared<dsp::stream<float>>();
    ScaleTestBlock blk(input, 1.0f);
    blk.stop();
    CHECK(blk.work_calls == 0);
}

TEST_CASE("Block stop unblocks a worker waiting for input", "[common][dsp][block]")
{
    auto input = std::make_shared<dsp::stream<float>>();
    ScaleTestBlock blk(input, 1.0f);
    blk.start();

    // Nothing is fed: the worker sits in read() until stop() wakes it up
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto fut = std::async(std::launch::async, [&]() { blk.stop(); });
    REQUIRE(fut.wait_for(dsp_test::kTimeout) == std::future_status::ready);
}

TEST_CASE("Block constructor creates a fresh output stream", "[common][dsp][block]")
{
    auto input = std::make_shared<dsp::stream<float>>();
    ScaleTestBlock a(input, 1.0f);
    ScaleTestBlock b(a.output_stream, 1.0f);

    CHECK(a.input_stream == input);
    CHECK(a.output_stream != nullptr);
    CHECK(b.input_stream == a.output_stream);
    CHECK(b.output_stream != a.output_stream);
}

TEST_CASE("Block chains pass data through every stage", "[common][dsp][block]")
{
    auto input = std::make_shared<dsp::stream<float>>();
    ScaleTestBlock a(input, 2.0f);
    ScaleTestBlock b(a.output_stream, -3.0f);

    std::vector<float> data(5000);
    for (size_t i = 0; i < data.size(); i++)
        data[i] = (float)(i % 17);

    dsp_test::Collector<float> col(b.output_stream);
    a.start();
    b.start();
    bool fed = dsp_test::feedStream(input, data, 512);
    bool got = fed && col.waitFor(data.size());
    a.stop();
    b.stop();
    auto out = col.finish();

    REQUIRE(fed);
    REQUIRE(got);
    REQUIRE(out.size() == data.size());
    size_t mismatches = 0, first = 0;
    for (size_t i = 0; i < data.size(); i++)
        if (!(out[i] == -6.0f * data[i]) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}
