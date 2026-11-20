#include "pipeline/modules/generic/module_soft2hard.h"
#include "../../pipeline_test_helpers.h"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <random>

using namespace pipeline_test_helpers;
namespace pl = satdump::pipeline;

namespace
{
    // Builds soft symbols (MSB first) for the given bytes. Bits set to 1 get a strictly
    // positive value, bits set to 0 get zero or a negative value.
    std::vector<uint8_t> softFromBytes(const std::vector<uint8_t> &bytes, uint32_t seed)
    {
        std::mt19937 rng(seed);
        std::vector<uint8_t> soft;
        for (uint8_t byte : bytes)
            for (int i = 7; i >= 0; i--)
            {
                int8_t v;
                if ((byte >> i) & 1)
                    v = (int8_t)(1 + rng() % 127); // 1..127
                else
                    v = (int8_t)(-(int)(rng() % 129)); // -128..0
                soft.push_back((uint8_t)v);
            }
        return soft;
    }

    std::vector<uint8_t> randomBytes(size_t n, uint32_t seed)
    {
        std::mt19937 rng(seed);
        std::vector<uint8_t> v(n);
        for (auto &b : v)
            b = rng() & 0xFF;
        return v;
    }

    std::shared_ptr<pl::ProcessingModule> runSoft2Hard(const std::string &input, const std::string &hint)
    {
        auto m = pl::generic::Soft2HardModule::getInstance(input, hint, {});
        m->setInputType(pl::DATA_FILE);
        m->setOutputType(pl::DATA_FILE);
        m->init();
        m->process();
        return m;
    }
} // namespace

TEST_CASE("Soft2HardModule identifies itself and supports streaming", "[pipeline][modules][soft2hard]")
{
    CHECK(pl::generic::Soft2HardModule::getID() == "soft2hard");
    CHECK(pl::generic::Soft2HardModule::getParams().is_null());

    auto m = pl::generic::Soft2HardModule::getInstance("in", "out", {});
    CHECK(m->getIDM() == "soft2hard");
    CHECK(m->getInputTypes() == std::vector<pl::ModuleDataType>{pl::DATA_FILE, pl::DATA_STREAM});
    CHECK(m->getOutputTypes() == std::vector<pl::ModuleDataType>{pl::DATA_FILE, pl::DATA_STREAM});
}

TEST_CASE("Soft2HardModule slices soft symbols into packed MSB-first bytes", "[pipeline][modules][soft2hard]")
{
    TempDir tmp("pipe_s2h");

    // 2 full output blocks of 256 bytes
    auto expected = randomBytes(512, 1234);
    expected[0] = 0x00;
    expected[1] = 0xFF;
    expected[2] = 0x80;
    expected[3] = 0x01;
    std::string input = tmp.file("in.soft");
    writeBytes(input, softFromBytes(expected, 99));

    auto m = runSoft2Hard(input, tmp.file("out"));

    CHECK(m->getOutput() == tmp.file("out") + ".bin");
    CHECK(readBytes(m->getOutput()) == expected);
}

TEST_CASE("Soft2HardModule treats a zero soft symbol as a 0 bit", "[pipeline][modules][soft2hard]")
{
    TempDir tmp("pipe_s2h");

    std::vector<uint8_t> soft(2048, 0); // all exactly 0
    for (int i = 0; i < 8; i++)
        soft[i] = 1; // first byte: smallest positive values -> 0xFF
    std::string input = tmp.file("in.soft");
    writeBytes(input, soft);

    auto m = runSoft2Hard(input, tmp.file("out"));
    auto out = readBytes(m->getOutput());
    REQUIRE(out.size() == 256);
    CHECK(out[0] == 0xFF);
    for (size_t i = 1; i < out.size(); i++)
        CHECK(out[i] == 0x00);
}

// module_soft2hard.cpp:52-64 : bytes are only written in blocks of 256 and the partial block left
// in byte_buffer is never flushed when the input ends, so up to 255 trailing bytes are lost.
TEST_CASE("Soft2HardModule writes trailing bytes that do not fill a 256-byte block", "[pipeline][modules][soft2hard][!shouldfail]")
{
    TempDir tmp("pipe_s2h");

    auto expected = randomBytes(256 + 32, 777);
    std::string input = tmp.file("in.soft");
    writeBytes(input, softFromBytes(expected, 5));

    auto m = runSoft2Hard(input, tmp.file("out"));
    auto out = readBytes(m->getOutput());
    CHECK(out.size() == expected.size());
    CHECK(std::equal(out.begin(), out.end(), expected.begin(), expected.end()));
}

TEST_CASE("Soft2HardModule converts a streamed input", "[pipeline][modules][soft2hard]")
{
    auto m = pl::generic::Soft2HardModule::getInstance("", "unused", {});
    m->input_fifo = std::make_shared<dsp::RingBuffer<uint8_t>>(1000000);
    m->output_fifo = std::make_shared<dsp::RingBuffer<uint8_t>>(1000000);
    m->setInputType(pl::DATA_STREAM);
    m->setOutputType(pl::DATA_STREAM);
    m->init();
    m->input_active = true;

    std::thread t([m]() { m->process(); });

    auto expected = randomBytes(256, 4242);
    auto soft = softFromBytes(expected, 17);
    m->input_fifo->write(soft.data(), soft.size());

    bool got = waitFor([&]() { return m->output_fifo->getReadable() >= 256; });

    m->input_active = false;
    m->input_fifo->stopReader();
    m->input_fifo->stopWriter();
    t.join();

    REQUIRE(got);
    std::vector<uint8_t> out(256);
    m->output_fifo->read(out.data(), 256);
    CHECK(out == expected);
}
