#include "common/ziq.h"
#include "helpers/temp_files.h"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <vector>

namespace
{
    using test_helpers::TempFile;
    using test_helpers::writeBytes;

    // Raw ZIQ header: signature, compressed flag, bits, samplerate, annotation size, annotation
    std::vector<uint8_t> ziqHeader(const char *sig, bool compressed, uint8_t bits, uint64_t samplerate, const std::string &annotation)
    {
        std::vector<uint8_t> out(sig, sig + 4);
        out.push_back(compressed);
        out.push_back(bits);
        for (int i = 0; i < 8; i++)
            out.push_back(samplerate >> (8 * i));
        uint64_t len = annotation.size();
        for (int i = 0; i < 8; i++)
            out.push_back(len >> (8 * i));
        out.insert(out.end(), annotation.begin(), annotation.end());
        return out;
    }
} // namespace

TEST_CASE("ziq::isValidZIQ checks the file signature", "[common][ziq]")
{
    TempFile good("good.ziq");
    writeBytes(good.path, ziqHeader("ZIQ_", true, 8, 1000, ""));
    CHECK(ziq::isValidZIQ(good.path.string()));

    TempFile bad("bad.ziq");
    writeBytes(bad.path, ziqHeader("ZIQ2", true, 8, 1000, ""));
    CHECK_FALSE(ziq::isValidZIQ(bad.path.string()));
}

TEST_CASE("ziq::getCfgFromFile parses the header", "[common][ziq]")
{
    TempFile f("cfg.ziq");
    writeBytes(f.path, ziqHeader("ZIQ_", false, 16, 6000000, R"({"frequency":137100000})"));

    ziq::ziq_cfg cfg = ziq::getCfgFromFile(f.path.string());

    CHECK_FALSE(cfg.is_compressed);
    CHECK(cfg.bits_per_sample == 16);
    CHECK(cfg.samplerate == 6000000);
    CHECK(cfg.annotation == R"({"frequency":137100000})");
}

TEST_CASE("ziq::ziq_writer writes a header getCfgFromFile can read", "[common][ziq]")
{
    TempFile f("writer.ziq");
    {
        std::ofstream stream(f.path, std::ios::binary);
        ziq::ziq_cfg cfg;
        cfg.is_compressed = true; // The uncompressed writer path is not safe to construct, see UNTESTED_ISSUES.md
        cfg.bits_per_sample = 8;
        cfg.samplerate = 2400000;
        cfg.annotation = "hello";
        ziq::ziq_writer writer(cfg, stream);
    }

    REQUIRE(ziq::isValidZIQ(f.path.string()));
    ziq::ziq_cfg cfg = ziq::getCfgFromFile(f.path.string());
    CHECK(cfg.is_compressed);
    CHECK(cfg.bits_per_sample == 8);
    CHECK(cfg.samplerate == 2400000);
    CHECK(cfg.annotation == "hello");
    CHECK(std::filesystem::file_size(f.path) >= 4 + 1 + 1 + 8 + 8 + 5);
}

// Bug: ziq.cpp - the compressed writer only ever calls ZSTD_compressStream2 with
// ZSTD_e_continue and never flushes/ends the frame (not even in the destructor),
// so the tail of every recording (here: all of it) is lost.
TEST_CASE("ziq::ziq_writer and ziq_reader round-trip compressed samples", "[common][ziq][!shouldfail]")
{
    TempFile f("roundtrip.ziq");
    const int n = 1000;
    std::vector<complex_t> input(n);
    for (int i = 0; i < n; i++)
        input[i] = complex_t(((i * 37) % 255 - 127) / 127.0f, ((i * 91) % 255 - 127) / 127.0f);

    {
        std::ofstream stream(f.path, std::ios::binary);
        ziq::ziq_cfg cfg;
        cfg.is_compressed = true;
        cfg.bits_per_sample = 32; // Float, so a failed read leaves the output untouched
        cfg.samplerate = 1000000;
        cfg.annotation = "";
        ziq::ziq_writer writer(cfg, stream);
        writer.write(input.data(), n);
    }

    std::vector<complex_t> output(n, complex_t(9, 9));
    {
        std::ifstream stream(f.path, std::ios::binary);
        ziq::ziq_reader reader(stream);
        reader.read(output.data(), n);
    }

    bool all_match = true;
    for (int i = 0; i < n; i++)
        if (output[i].real != input[i].real || output[i].imag != input[i].imag)
            all_match = false;
    CHECK(all_match);
}

// Bug: ziq.cpp - the reader constructor sets isValid = false on a bad signature,
// then unconditionally overwrites it with true, so read() never reports the error.
TEST_CASE("ziq::ziq_reader read fails on a file with a bad signature", "[common][ziq][!shouldfail]")
{
    TempFile f("badsig.ziq");
    writeBytes(f.path, ziqHeader("NOPE", true, 32, 1000, ""));

    std::ifstream stream(f.path, std::ios::binary);
    ziq::ziq_reader reader(stream);
    complex_t out[4];
    CHECK(reader.read(out, 4) == 1);
}
