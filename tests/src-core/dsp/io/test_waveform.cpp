#include "dsp/io/waveform.h"
#include "dsp/ndsp_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;
using namespace satdump::ndsp;

namespace
{
    // Collects at least n samples from a waveform source, then stops it
    template <typename T>
    ndsp_test::OutputResult<T> runWaveform(std::shared_ptr<WaveformBlock<T>> b, size_t n)
    {
        ndsp_test::OutputResult<T> r;
        ndsp_test::Collector<T> col(b->get_output(0, 4).fifo);
        b->start();
        bool got = col.waitFor(n);
        REQUIRE(ndsp_test::stopBlock(b, true));
        REQUIRE(col.waitForTerminator());
        col.finish();
        REQUIRE(got);
        r.data = col.data();
        r.sizes = col.sizes();
        r.terminators = col.terminators();
        return r;
    }
} // namespace

TEST_CASE("ndsp WaveformBlock generates a cosine", "[dsp][io][waveform]")
{
    auto b = std::make_shared<WaveformBlock<float>>();
    REQUIRE(ndsp_test::configure(*b, {{"samprate", 48000.0}, {"freq", 1000.0}, {"amp", 0.5}}) == Block::RES_OK);
    auto r = runWaveform(b, 8192);
    REQUIRE(r.sizes.size() >= 1);
    CHECK(r.sizes[0] == 8192);
    // Within the first buffer, out[i] = amp * cos(2 pi f i / fs) plus very low noise.
    // The block computes t = i / fs as a float, hence the 1e-3 tolerance.
    size_t mismatches = 0, first = 0;
    for (size_t i = 0; i < 8192; i++)
        if (!(std::fabs(r.data[i] - 0.5 * cos(2 * M_PI * 1000.0 * i / 48000.0)) < 1e-3) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
    CHECK(r.terminators == 1);
}

TEST_CASE("ndsp WaveformBlock complex output is an analytic tone", "[dsp][io][waveform]")
{
    for (std::string wf : {"cosine", "sine"})
    {
        INFO(wf);
        auto b = std::make_shared<WaveformBlock<complex_t>>();
        REQUIRE(ndsp_test::configure(*b, {{"waveform", wf}, {"samprate", 10000.0}, {"freq", 1250.0}, {"phase", 0.5}}) == Block::RES_OK);
        auto r = runWaveform(b, 8192);
        size_t mismatches = 0, first = 0;
        for (size_t i = 0; i < 8192; i++)
        {
            double p = 2 * M_PI * 1250.0 * i / 10000.0 + 0.5;
            if (!(ndsp_test::absDiff(r.data[i], complex_t(cos(p), sin(p))) < 1e-3) && mismatches++ == 0)
                first = i;
        }
        INFO("first mismatch at sample " << first);
        REQUIRE(mismatches == 0);
        // Positive frequency of 2 pi / 8 rad/sample
        CHECK_THAT(ndsp_test::estimateFreq(r.data, 0, 8192), WithinAbs(2 * M_PI / 8, 1e-4));
    }
}

TEST_CASE("ndsp WaveformBlock sine for float output", "[dsp][io][waveform]")
{
    auto b = std::make_shared<WaveformBlock<float>>();
    REQUIRE(ndsp_test::configure(*b, {{"waveform", "sine"}, {"samprate", 8000.0}, {"freq", 500.0}, {"amp", 2.0}}) == Block::RES_OK);
    auto r = runWaveform(b, 8192);
    size_t mismatches = 0, first = 0;
    for (size_t i = 0; i < 8192; i++)
        if (!(std::fabs(r.data[i] - 2.0 * sin(2 * M_PI * 500.0 * i / 8000.0)) < 2e-3) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}

TEST_CASE("ndsp WaveformBlock config round-trip", "[dsp][io][waveform]")
{
    WaveformBlock<float> b;
    Block &bb = b;
    CHECK(b.d_id == "waveform_f");
    CHECK(WaveformBlock<complex_t>().d_id == "waveform_c");
    CHECK(b.is_async());
    REQUIRE(bb.set_cfg(nlohmann::json{{"waveform", "sine"}, {"samprate", 1e6}, {"freq", 1234.0}, {"amp", 0.25}, {"phase", 1.5}, {"bufs", 1024}}) == Block::RES_OK);
    nlohmann::json all = bb.get_cfg();
    CHECK(all["waveform"] == "sine");
    CHECK(all["samprate"] == 1e6);
    CHECK(all["freq"] == 1234.0);
    CHECK(all["amp"] == 0.25);
    CHECK(all["phase"] == 1.5);
    CHECK(all["bufs"] == 1024);
    // "waveform" is declared twice, the second (string) declaration wins
    CHECK(b.get_cfg_list()["waveform"]["type"] == "string");
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

// Bug: waveform.cpp:12,16-23, d_buffer_size is only set in the constructor and
// init() does not copy p_buffer_size, so the "bufs" setting is never applied
TEST_CASE("ndsp WaveformBlock applies the buffer size setting", "[dsp][io][waveform][!shouldfail]")
{
    auto b = std::make_shared<WaveformBlock<float>>();
    b->set_cfg("bufs", 256);
    auto r = runWaveform(b, 256);
    REQUIRE_FALSE(r.sizes.empty());
    CHECK(r.sizes[0] == 256);
}

// Bug: waveform.cpp:53,85 computes t = i / samprate from the start of every
// buffer and never advances d_phase, so each buffer restarts the waveform and
// the signal jumps at buffer boundaries (here 8192 * 1000 / 48000 is not an
// integer number of periods)
TEST_CASE("ndsp WaveformBlock is phase-continuous across buffers", "[dsp][io][waveform][!shouldfail]")
{
    auto b = std::make_shared<WaveformBlock<float>>();
    REQUIRE(ndsp_test::configure(*b, {{"samprate", 48000.0}, {"freq", 1000.0}}) == Block::RES_OK);
    auto r = runWaveform(b, 2 * 8192);
    REQUIRE(r.data.size() >= 2 * 8192);
    double max_err = 0;
    for (size_t i = 0; i < 2 * 8192; i++)
        max_err = std::max(max_err, std::fabs(r.data[i] - cos(2 * M_PI * 1000.0 * i / 48000.0)));
    CHECK(max_err < 1e-3);
}
