#include "core/exception.h"
#include "pipeline/modules/ccsds/module_ccsds_simple_psk_decoder.h"
#include "pipeline/pipeline_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <random>

using namespace pipeline_test;
namespace pl = satdump::pipeline;

namespace
{
    const int CADU_BITS = 1024;
    const int CADU_BYTES = CADU_BITS / 8;

    // CCSDS pseudo-randomizer sequence, h(x) = x^8 + x^7 + x^5 + x^3 + 1, all-ones seed
    std::vector<uint8_t> ccsdsPN(size_t n)
    {
        int s[8] = {1, 1, 1, 1, 1, 1, 1, 1};
        std::vector<uint8_t> out(n, 0);
        for (size_t i = 0; i < n * 8; i++)
        {
            int bit = s[0];
            int fb = s[0] ^ s[3] ^ s[5] ^ s[7];
            for (int k = 0; k < 7; k++)
                s[k] = s[k + 1];
            s[7] = fb;
            out[i / 8] = out[i / 8] << 1 | bit;
        }
        return out;
    }

    // Frames with the standard ASM followed by random payload
    std::vector<std::vector<uint8_t>> makeCadus(int count, uint32_t seed)
    {
        std::mt19937 rng(seed);
        std::vector<std::vector<uint8_t>> cadus;
        for (int f = 0; f < count; f++)
        {
            std::vector<uint8_t> c(CADU_BYTES);
            c[0] = 0x1A, c[1] = 0xCF, c[2] = 0xFC, c[3] = 0x1D;
            for (int i = 4; i < CADU_BYTES; i++)
                c[i] = rng() & 0xFF;
            cadus.push_back(c);
        }
        return cadus;
    }

    // BPSK soft symbols (one int8 per bit). Optionally inverted, plus zero padding.
    std::vector<uint8_t> toSoft(const std::vector<std::vector<uint8_t>> &cadus, bool invert, int pad_symbols, uint32_t seed)
    {
        std::mt19937 rng(seed);
        std::vector<uint8_t> soft;
        for (auto &c : cadus)
            for (uint8_t byte : c)
                for (int i = 7; i >= 0; i--)
                {
                    bool bit = ((byte >> i) & 1) ^ invert;
                    int mag = 40 + rng() % 80;
                    soft.push_back((uint8_t)(int8_t)(bit ? mag : -mag));
                }
        for (int i = 0; i < pad_symbols; i++)
            soft.push_back(0);
        return soft;
    }

    std::vector<uint8_t> concat(const std::vector<std::vector<uint8_t>> &frames)
    {
        std::vector<uint8_t> out;
        for (auto &f : frames)
            out.insert(out.end(), f.begin(), f.end());
        return out;
    }

    nlohmann::json bpskParams() { return {{"constellation", "bpsk"}, {"cadu_size", CADU_BITS}, {"rs_i", 0}, {"derandomize", false}}; }

    std::shared_ptr<pl::ProcessingModule> runDecoder(const std::string &input, const std::string &hint, nlohmann::json params)
    {
        auto m = pl::ccsds::CCSDSSimplePSKDecoderModule::getInstance(input, hint, params);
        m->setInputType(pl::DATA_FILE);
        m->setOutputType(pl::DATA_FILE);
        m->init();
        m->process();
        return m;
    }
} // namespace

TEST_CASE("ccsdsPN test helper matches the published CCSDS randomizer sequence", "[pipeline][modules][ccsds_simple_psk_decoder]")
{
    auto pn = ccsdsPN(10);
    CHECK(pn == std::vector<uint8_t>{0xFF, 0x48, 0x0E, 0xC0, 0x9A, 0x0D, 0x70, 0xBC, 0x8E, 0x2C});
}

TEST_CASE("CCSDSSimplePSKDecoderModule identifies itself", "[pipeline][modules][ccsds_simple_psk_decoder]")
{
    CHECK(pl::ccsds::CCSDSSimplePSKDecoderModule::getID() == "ccsds_simple_psk_decoder");
    auto m = pl::ccsds::CCSDSSimplePSKDecoderModule::getInstance("in", "out", bpskParams());
    CHECK(m->getIDM() == "ccsds_simple_psk_decoder");
    CHECK(m->getInputTypes() == std::vector<pl::ModuleDataType>{pl::DATA_FILE, pl::DATA_STREAM});
    CHECK(m->getOutputTypes() == std::vector<pl::ModuleDataType>{pl::DATA_FILE, pl::DATA_STREAM});
}

TEST_CASE("CCSDSSimplePSKDecoderModule rejects invalid configurations", "[pipeline][modules][ccsds_simple_psk_decoder]")
{
    auto params = bpskParams();

    SECTION("unknown constellation")
    {
        params["constellation"] = "8psk";
        CHECK_THROWS_AS(pl::ccsds::CCSDSSimplePSKDecoderModule::getInstance("in", "out", params), satdump::satdump_exception_t);
    }
    SECTION("unknown Reed-Solomon type")
    {
        params["rs_i"] = 4;
        params["rs_type"] = "rs255";
        CHECK_THROWS_AS(pl::ccsds::CCSDSSimplePSKDecoderModule::getInstance("in", "out", params), satdump::satdump_exception_t);
    }
    SECTION("Reed-Solomon enabled without a type")
    {
        params["rs_i"] = 4;
        CHECK_THROWS_AS(pl::ccsds::CCSDSSimplePSKDecoderModule::getInstance("in", "out", params), satdump::satdump_exception_t);
    }
    SECTION("missing mandatory parameter")
    {
        params.erase("cadu_size");
        CHECK_THROWS(pl::ccsds::CCSDSSimplePSKDecoderModule::getInstance("in", "out", params));
    }
}

TEST_CASE("CCSDSSimplePSKDecoderModule extracts BPSK CADUs from soft symbols", "[pipeline][modules][ccsds_simple_psk_decoder]")
{
    TempDir tmp("pipe_ccsds");
    auto cadus = makeCadus(12, 1);
    std::string input = tmp.file("in.soft");
    // Trailing padding keeps the input length off a frame boundary
    writeBytes(input, toSoft(cadus, false, CADU_BITS / 2, 2));

    auto m = runDecoder(input, tmp.file("out"), bpskParams());

    CHECK(m->getOutput() == tmp.file("out") + ".cadu");
    CHECK(readBytes(m->getOutput()) == concat(cadus));

    auto stats = m->getModuleStats();
    CHECK(stats["deframer_lock"].is_boolean());
    CHECK(stats["deframer_state"].is_string());
    CHECK_FALSE(stats.contains("rs_avg"));
}

TEST_CASE("CCSDSSimplePSKDecoderModule corrects inverted BPSK polarity", "[pipeline][modules][ccsds_simple_psk_decoder]")
{
    TempDir tmp("pipe_ccsds");
    auto cadus = makeCadus(5, 3);
    std::string input = tmp.file("in.soft");
    writeBytes(input, toSoft(cadus, true, CADU_BITS / 2, 4));

    auto m = runDecoder(input, tmp.file("out"), bpskParams());
    CHECK(readBytes(m->getOutput()) == concat(cadus));
}

TEST_CASE("CCSDSSimplePSKDecoderModule ignores data before the first sync marker", "[pipeline][modules][ccsds_simple_psk_decoder]")
{
    TempDir tmp("pipe_ccsds");
    auto cadus = makeCadus(4, 5);
    std::string input = tmp.file("in.soft");

    // 300 symbols of zeros (bit 0) first, so frames are not aligned with the read buffer
    std::vector<uint8_t> soft(300, (uint8_t)(int8_t)-100);
    auto frames = toSoft(cadus, false, CADU_BITS / 2, 6);
    soft.insert(soft.end(), frames.begin(), frames.end());
    writeBytes(input, soft);

    auto m = runDecoder(input, tmp.file("out"), bpskParams());
    CHECK(readBytes(m->getOutput()) == concat(cadus));
}

TEST_CASE("CCSDSSimplePSKDecoderModule derandomizes from byte 4 by default", "[pipeline][modules][ccsds_simple_psk_decoder]")
{
    TempDir tmp("pipe_ccsds");
    auto plain = makeCadus(3, 7);

    // Randomize the payload (everything after the ASM) as a CCSDS transmitter would
    auto pn = ccsdsPN(CADU_BYTES - 4);
    auto sent = plain;
    for (auto &c : sent)
        for (int i = 4; i < CADU_BYTES; i++)
            c[i] ^= pn[i - 4];

    std::string input = tmp.file("in.soft");
    writeBytes(input, toSoft(sent, false, CADU_BITS / 2, 8));

    auto params = bpskParams();
    params.erase("derandomize"); // default is on
    auto m = runDecoder(input, tmp.file("out"), params);
    CHECK(readBytes(m->getOutput()) == concat(plain));
}

TEST_CASE("CCSDSSimplePSKDecoderModule supports a custom ASM and .frm output", "[pipeline][modules][ccsds_simple_psk_decoder]")
{
    TempDir tmp("pipe_ccsds");
    auto cadus = makeCadus(4, 9);
    for (auto &c : cadus)
        c[0] = 0x12, c[1] = 0x34, c[2] = 0x56, c[3] = 0x78;

    std::string input = tmp.file("in.soft");
    writeBytes(input, toSoft(cadus, false, CADU_BITS / 2, 10));

    auto params = bpskParams();
    params["asm"] = "12345678";
    params["ccsds"] = false;
    auto m = runDecoder(input, tmp.file("out"), params);

    CHECK(m->getOutput() == tmp.file("out") + ".frm");
    CHECK(readBytes(m->getOutput()) == concat(cadus));
}

TEST_CASE("CCSDSSimplePSKDecoderModule outputs nothing without a sync marker", "[pipeline][modules][ccsds_simple_psk_decoder]")
{
    TempDir tmp("pipe_ccsds");
    std::mt19937 rng(11);
    std::vector<uint8_t> soft(CADU_BITS * 4 + 100);
    for (auto &s : soft)
        s = (uint8_t)(int8_t)((rng() & 1) ? 100 : -100);
    std::string input = tmp.file("in.soft");
    writeBytes(input, soft);

    auto m = runDecoder(input, tmp.file("out"), bpskParams());
    REQUIRE(std::filesystem::exists(m->getOutput()));
    CHECK(std::filesystem::file_size(m->getOutput()) == 0);
}

// Bug: module_ccsds_simple_psk_decoder.cpp:135-140 : the loop runs while !data_in.eof(); when the input
// length is an exact multiple of the read size, the final read returns nothing but the previous
// (stale) buffer is processed again, so the last CADU is emitted twice.
TEST_CASE("CCSDSSimplePSKDecoderModule emits each CADU once for frame-aligned input", "[pipeline][modules][ccsds_simple_psk_decoder][!shouldfail]")
{
    TempDir tmp("pipe_ccsds");
    auto cadus = makeCadus(6, 13);
    std::string input = tmp.file("in.soft");
    writeBytes(input, toSoft(cadus, false, 0, 14));

    auto m = runDecoder(input, tmp.file("out"), bpskParams());
    auto out = readBytes(m->getOutput());
    CHECK(out.size() / CADU_BYTES == cadus.size());
}
