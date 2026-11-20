#include "dsp/io/iq_sink.h"
#include "dsp/io/iq_source.h"
#include "dsp/io/iq_types.h"
#include "dsp/ndsp_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using namespace satdump::ndsp;

///////////////////////////////////////////////////////////////////////////////
// IQType
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp IQType converts to and from strings", "[dsp][io][iq_types]")
{
    struct Case
    {
        IQTypeEnum e;
        std::string name;
        std::string alias;
    };
    for (auto &c : std::vector<Case>{{CF32, "cf32", "f32"}, {CS32, "cs32", "s32"}, {CS16, "cs16", "s16"}, {CS8, "cs8", "s8"}, {CU8, "cu8", "u8"}, {WAV16, "wav16", "wav16"}})
    {
        INFO(c.name);
        CHECK((std::string)IQType(c.e) == c.name);
        CHECK((IQTypeEnum)IQType(c.name) == c.e);
        CHECK((IQTypeEnum)IQType(c.alias) == c.e);
        CHECK((IQTypeEnum)IQType(c.name.c_str()) == c.e);
    }
}

TEST_CASE("ndsp IQType defaults, assignment and errors", "[dsp][io][iq_types]")
{
    IQType t;
    CHECK(t == CF32);
    t = CS8;
    CHECK(t == CS8);
    t = std::string("cu8");
    CHECK(t == CU8);
    t = "cs16";
    CHECK(t == CS16);
    CHECK_THROWS_AS(IQType("ziq"), satdump::satdump_exception_t);
    CHECK_THROWS_AS(IQType(""), satdump::satdump_exception_t);
    CHECK_THROWS_AS((std::string)IQType((IQTypeEnum)99), satdump::satdump_exception_t);
    // A failed assignment keeps the previous value
    CHECK_THROWS(t = "bogus");
    CHECK(t == CS16);
}

///////////////////////////////////////////////////////////////////////////////
// IQ sink
///////////////////////////////////////////////////////////////////////////////

namespace
{
    std::vector<uint8_t> runIQSink(const std::string &type, const std::vector<complex_t> &data, size_t &raw)
    {
        ndsp_test::TempFile tf;
        auto b = std::make_shared<IQSinkBlock>();
        REQUIRE(ndsp_test::configure(*b, {{"file", tf.str()}, {"type", type}}) == Block::RES_OK);
        auto r = ndsp_test::runSimple<complex_t, complex_t>(b, data, 100);
        REQUIRE(r.completed);
        raw = b->total_written_raw;
        return ndsp_test::readBytes<uint8_t>(tf.str());
    }
} // namespace

TEST_CASE("ndsp IQSinkBlock writes cf32 unchanged", "[dsp][io][iq_sink]")
{
    auto data = ndsp_test::randomComplex(250, 63);
    size_t raw = 0;
    auto bytes = runIQSink("cf32", data, raw);
    REQUIRE(bytes.size() == data.size() * sizeof(complex_t));
    CHECK(raw == bytes.size());
    CHECK(memcmp(bytes.data(), data.data(), bytes.size()) == 0);
}

TEST_CASE("ndsp IQSinkBlock converts to cs16", "[dsp][io][iq_sink]")
{
    std::vector<complex_t> data = {{0, 1}, {-1, 0.5}, {0.25, -0.25}, {2, -2}};
    size_t raw = 0;
    auto bytes = runIQSink("cs16", data, raw);
    REQUIRE(bytes.size() == data.size() * 2 * sizeof(int16_t));
    CHECK(raw == bytes.size());
    std::vector<int16_t> s(data.size() * 2);
    memcpy(s.data(), bytes.data(), bytes.size());
    std::vector<int16_t> expected = {0, 32767, -32767, 16384, 8192, -8192, 32767, -32768};
    for (size_t i = 0; i < s.size(); i++)
    {
        INFO("i = " << i);
        // Rounding differs between VOLK kernels for exact halves (16383.5)
        CHECK(std::abs(s[i] - expected[i]) <= 1);
    }
}

TEST_CASE("ndsp IQSinkBlock converts to cs8", "[dsp][io][iq_sink]")
{
    std::vector<complex_t> data = {{0, 1}, {-1, 0.5}, {3, -3}};
    size_t raw = 0;
    auto bytes = runIQSink("cs8", data, raw);
    REQUIRE(bytes.size() == data.size() * 2);
    CHECK(raw == bytes.size());
    std::vector<int8_t> s(bytes.begin(), bytes.end());
    std::vector<int8_t> expected = {0, 127, -127, 64, 127, -128};
    for (size_t i = 0; i < s.size(); i++)
        CHECK(std::abs(s[i] - expected[i]) <= 1);
}

TEST_CASE("ndsp IQSinkBlock converts to cs32", "[dsp][io][iq_sink]")
{
    std::vector<complex_t> data = {{0, 0.5}, {-0.25, 0.125}};
    size_t raw = 0;
    auto bytes = runIQSink("cs32", data, raw);
    REQUIRE(bytes.size() == data.size() * 2 * sizeof(int32_t));
    CHECK(raw == bytes.size());
    std::vector<int32_t> s(4);
    memcpy(s.data(), bytes.data(), bytes.size());
    CHECK(s == std::vector<int32_t>{0, 1 << 30, -(1 << 29), 1 << 28});
}

TEST_CASE("ndsp IQSinkBlock throws on start when the file cannot be opened", "[dsp][io][iq_sink]")
{
    IQSinkBlock b;
    b.set_cfg("file", (std::filesystem::temp_directory_path() / "satdump_ndsp_no_such_dir" / "a" / "b.cf32").string());
    CHECK_THROWS_AS(b.start(), satdump::satdump_exception_t);
    b.stop(); // Nothing was started
}

TEST_CASE("ndsp IQSinkBlock config round-trip", "[dsp][io][iq_sink]")
{
    IQSinkBlock b;
    Block &bb = b;
    CHECK(b.d_id == "iq_sink_c");
    CHECK(b.get_outputs().empty());
    REQUIRE(bb.set_cfg(nlohmann::json{{"file", "/a/b"}, {"type", "cs8"}, {"buffer_size", 4096}, {"autogen", true}, {"samplerate", 2.4e6}, {"frequency", 137.1e6}, {"timestamp", 1700000000.5}}) ==
            Block::RES_OK);
    nlohmann::json all = bb.get_cfg();
    CHECK(all.size() == 7);
    CHECK(all["file"] == "/a/b");
    CHECK(all["type"] == "cs8");
    CHECK(all["buffer_size"] == 4096);
    CHECK(all["autogen"] == true);
    CHECK(all["samplerate"] == 2.4e6);
    CHECK(all["frequency"] == 137.1e6);
    CHECK(all["timestamp"] == 1700000000.5);
    CHECK(b.get_cfg_list()["type"]["disable"] == false);
    CHECK(bb.set_cfg(nlohmann::json{{"type", "nope"}}) == Block::RES_ERR);
    CHECK(b.get_cfg("type") == "cs8");
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

///////////////////////////////////////////////////////////////////////////////
// IQ source
///////////////////////////////////////////////////////////////////////////////

namespace
{
    std::vector<complex_t> runIQSource(const std::string &path, const std::string &type, int bufsize, bool swap = false)
    {
        auto b = std::make_shared<IQSourceBlock>();
        REQUIRE(ndsp_test::configure(*b, {{"file", path}, {"type", type}, {"buffer_size", bufsize}, {"iq_swap", swap}}) == Block::RES_OK);
        ndsp_test::Collector<complex_t> col(b->get_output(0, 4).fifo);
        b->start();
        bool got = col.waitForTerminator();
        REQUIRE(ndsp_test::stopBlock(b));
        col.finish();
        REQUIRE(got);
        CHECK(col.propagatingTerminators() == 1);
        return col.data();
    }
} // namespace

TEST_CASE("ndsp IQSourceBlock reads cf32 files", "[dsp][io][iq_source]")
{
    ndsp_test::TempFile tf;
    auto data = ndsp_test::randomComplex(1000, 64);
    ndsp_test::writeBytes(tf.str(), data);
    auto out = runIQSource(tf.str(), "cf32", 250);
    // As in FileSourceBlock, extra stale samples follow (see the bug test below)
    REQUIRE(out.size() >= data.size());
    size_t mismatches = 0, first = 0;
    for (size_t i = 0; i < data.size(); i++)
        if (!ndsp_test::sameBits(out[i], data[i]) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}

TEST_CASE("ndsp IQSourceBlock converts cs16 and cs8 files", "[dsp][io][iq_source]")
{
    ndsp_test::TempFile t16, t8;
    ndsp_test::writeBytes(t16.str(), std::vector<int16_t>{32767, -32767, 0, 16384});
    ndsp_test::writeBytes(t8.str(), std::vector<int8_t>{127, -127, 0, 64});

    auto o16 = runIQSource(t16.str(), "cs16", 2);
    REQUIRE(o16.size() >= 2);
    CHECK(o16[0].real == 1.0f);
    CHECK(o16[0].imag == -1.0f);
    CHECK(o16[1].real == 0.0f);
    CHECK_THAT(o16[1].imag, WithinAbs(16384 / 32767.0f, 1e-6));

    auto o8 = runIQSource(t8.str(), "cs8", 2);
    REQUIRE(o8.size() >= 2);
    CHECK(o8[0].real == 1.0f);
    CHECK(o8[0].imag == -1.0f);
    CHECK_THAT(o8[1].imag, WithinAbs(64 / 127.0f, 1e-6));
}

TEST_CASE("ndsp IQSourceBlock swaps I and Q when asked", "[dsp][io][iq_source]")
{
    ndsp_test::TempFile tf;
    std::vector<complex_t> data = {{1, 2}, {3, 4}};
    ndsp_test::writeBytes(tf.str(), data);
    auto out = runIQSource(tf.str(), "cf32", 2, true);
    REQUIRE(out.size() >= 2);
    CHECK(out[0].real == 2.0f);
    CHECK(out[0].imag == 1.0f);
    CHECK(out[1].real == 4.0f);
    CHECK(out[1].imag == 3.0f);
}

TEST_CASE("ndsp IQSourceBlock with a missing file only sends a terminator", "[dsp][io][iq_source]")
{
    ndsp_test::TempFile tf;
    auto out = runIQSource(tf.str(), "cs16", 64);
    CHECK(out.empty());
}

// Bug: iq_source.cpp:24 uses the count returned by
// BasebandReader::read_samples (common/dsp/io/baseband_interface.h:233), which
// is always the requested size, so short reads at EOF output stale samples.
TEST_CASE("ndsp IQSourceBlock outputs exactly the samples in the file", "[dsp][io][iq_source][!shouldfail]")
{
    ndsp_test::TempFile tf;
    ndsp_test::writeBytes(tf.str(), ndsp_test::randomComplex(10, 65));
    auto out = runIQSource(tf.str(), "cf32", 4);
    CHECK(out.size() == 10);
}

TEST_CASE("ndsp IQSourceBlock config round-trip", "[dsp][io][iq_source]")
{
    IQSourceBlock b;
    Block &bb = b;
    CHECK(b.d_id == "iq_source_cc");
    CHECK(b.is_async());
    CHECK(b.get_cfg("type") == "cf32");
    REQUIRE(bb.set_cfg(nlohmann::json{{"file", "/x.cs8"}, {"type", "cs8"}, {"buffer_size", 1000}, {"iq_swap", true}}) == Block::RES_OK);
    nlohmann::json all = bb.get_cfg();
    CHECK(all.size() == 4);
    CHECK(all["file"] == "/x.cs8");
    CHECK(all["type"] == "cs8");
    CHECK(all["buffer_size"] == 1000);
    CHECK(all["iq_swap"] == true);
    CHECK(bb.set_cfg(nlohmann::json{{"type", "zzz"}}) == Block::RES_ERR);
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}
