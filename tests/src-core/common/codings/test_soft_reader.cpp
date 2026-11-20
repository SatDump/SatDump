#include "common/codings/soft_reader.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    std::string makeSoftString(int count)
    {
        std::string s(count, '\0');
        for (int i = 0; i < count; i++)
            s[i] = (char)(int8_t)((i * 13) % 255 - 127);
        return s;
    }
} // namespace

TEST_CASE("SoftSymbolReader reads soft symbols straight from a file stream", "[common][codings][soft_reader]")
{
    std::string content = makeSoftString(3000);
    std::istringstream in(content);
    std::shared_ptr<dsp::RingBuffer<uint8_t>> fifo;
    satdump::pipeline::ModuleDataType type = satdump::pipeline::DATA_FILE;
    nlohmann::json params = nlohmann::json::object();

    satdump::SoftSymbolReader reader(in, fifo, type, params);

    std::vector<int8_t> buf(1000);
    reader.readSoftSymbols(buf.data(), 1000);
    {
        size_t mismatches = 0, first = 0;
        for (int i = 0; i < 1000; i++)
            if (!(buf[i] == (int8_t)content[i]) && mismatches++ == 0)
                first = i;
        INFO("first mismatch at symbol " << first);
        REQUIRE(mismatches == 0);
    }

    // Reads continue where the last one stopped
    std::vector<int8_t> buf2(2000);
    reader.readSoftSymbols(buf2.data(), 2000);
    {
        size_t mismatches = 0, first = 0;
        for (int i = 0; i < 2000; i++)
            if (!(buf2[i] == (int8_t)content[1000 + i]) && mismatches++ == 0)
                first = i;
        INFO("first mismatch at symbol " << first);
        REQUIRE(mismatches == 0);
    }
    CHECK(in.tellg() == 3000);
}

TEST_CASE("SoftSymbolReader with hard_symbols explicitly false stays in soft mode", "[common][codings][soft_reader]")
{
    std::string content = makeSoftString(16);
    std::istringstream in(content);
    std::shared_ptr<dsp::RingBuffer<uint8_t>> fifo;
    satdump::pipeline::ModuleDataType type = satdump::pipeline::DATA_FILE;
    nlohmann::json params = {{"hard_symbols", false}};

    satdump::SoftSymbolReader reader(in, fifo, type, params);
    std::vector<int8_t> buf(16);
    reader.readSoftSymbols(buf.data(), 16);
    for (int i = 0; i < 16; i++)
        CHECK(buf[i] == (int8_t)content[i]);
}

TEST_CASE("SoftSymbolReader reads soft symbols from a ring buffer in stream mode", "[common][codings][soft_reader]")
{
    std::string content = makeSoftString(500);
    std::istringstream in; // unused in stream mode
    auto fifo = std::make_shared<dsp::RingBuffer<uint8_t>>(1000000);
    satdump::pipeline::ModuleDataType type = satdump::pipeline::DATA_STREAM;
    nlohmann::json params = nlohmann::json::object();

    fifo->write((uint8_t *)content.data(), content.size());

    satdump::SoftSymbolReader reader(in, fifo, type, params);
    std::vector<int8_t> buf(300);
    reader.readSoftSymbols(buf.data(), 300);
    for (int i = 0; i < 300; i++)
        REQUIRE(buf[i] == (int8_t)content[i]);

    reader.readSoftSymbols(buf.data(), 200);
    for (int i = 0; i < 200; i++)
        REQUIRE(buf[i] == (int8_t)content[300 + i]);
}
