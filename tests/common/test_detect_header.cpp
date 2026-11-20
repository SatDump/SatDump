#include "common/detect_header.h"
#include "common/wav.h"
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>

namespace
{
    std::filesystem::path uniqueTempDir()
    {
        std::mt19937 rng(std::random_device{}());
        auto p = std::filesystem::temp_directory_path() / ("satdump_test_detect_header_" + std::to_string(rng()));
        std::filesystem::create_directories(p);
        return p;
    }

    // Temp directory removed with its contents at the end of each test
    struct TempDir
    {
        std::filesystem::path path = uniqueTempDir();
        ~TempDir() { std::filesystem::remove_all(path); }

        std::string write(const std::string &name, const std::vector<uint8_t> &data)
        {
            auto p = path / name;
            std::ofstream f(p, std::ios::binary);
            f.write((const char *)data.data(), data.size());
            return p.string();
        }
    };

    std::vector<uint8_t> wavFile(uint16_t format, uint16_t bits, uint32_t samplerate)
    {
        wav::WavHeader h;
        memset(&h, 0, sizeof(h));
        memcpy(h.RIFF, "RIFF", 4);
        memcpy(h.WAVE, "WAVE", 4);
        memcpy(h.fmt, "fmt ", 4);
        memcpy(h.data, "data", 4);
        h.sub_chunk_size = 16;
        h.audio_Format = format;
        h.channel_cnt = 2;
        h.samplerate = samplerate;
        h.bits_per_sample = bits;
        std::vector<uint8_t> out(sizeof(h) + 64, 0);
        memcpy(out.data(), &h, sizeof(h));
        return out;
    }

    std::vector<uint8_t> rf64File(uint16_t format, uint16_t bits, uint32_t samplerate)
    {
        wav::RF64Header h;
        memset(&h, 0, sizeof(h));
        memcpy(h.RIFF, "RF64", 4);
        memcpy(h.WAVE, "WAVE", 4);
        memcpy(h.fmt, "fmt ", 4);
        h.audio_Format = format;
        h.bits_per_sample = bits;
        h.samplerate = samplerate;
        std::vector<uint8_t> out(sizeof(h) + 64, 0);
        memcpy(out.data(), &h, sizeof(h));
        return out;
    }

    std::vector<uint8_t> ziqFile(uint64_t samplerate)
    {
        std::vector<uint8_t> out = {'Z', 'I', 'Q', '_', 1, 16};
        for (int i = 0; i < 8; i++)
            out.push_back(samplerate >> (8 * i));
        for (int i = 0; i < 8; i++)
            out.push_back(0);
        out.resize(out.size() + 64, 0);
        return out;
    }
} // namespace

TEST_CASE("try_parse_header detects WAV sample formats", "[common][detect_header]")
{
    TempDir dir;

    struct Case
    {
        uint16_t format, bits;
        std::string type;
    };
    for (Case c : {Case{1, 8, "cu8"}, Case{1, 16, "cs16"}, Case{1, 32, "cs32"}, Case{3, 32, "cf32"}})
    {
        HeaderInfo info = try_parse_header(dir.write("test_" + c.type + ".wav", wavFile(c.format, c.bits, 48000)));
        INFO(c.type);
        CHECK(info.valid);
        CHECK(info.type == c.type);
        CHECK(info.samplerate == 48000);
    }
}

TEST_CASE("try_parse_header rejects unsupported WAV formats", "[common][detect_header]")
{
    TempDir dir;
    CHECK_FALSE(try_parse_header(dir.write("pcm24.wav", wavFile(1, 24, 48000))).valid);
    CHECK_FALSE(try_parse_header(dir.write("float64.wav", wavFile(3, 64, 48000))).valid);
}

TEST_CASE("try_parse_header detects RF64 sample formats", "[common][detect_header]")
{
    TempDir dir;

    HeaderInfo info = try_parse_header(dir.write("big.wav", rf64File(1, 16, 6000000)));
    CHECK(info.valid);
    CHECK(info.type == "cs16");
    CHECK(info.samplerate == 6000000);

    info = try_parse_header(dir.write("bigf.wav", rf64File(3, 32, 10000000)));
    CHECK(info.valid);
    CHECK(info.type == "cf32");
    CHECK(info.samplerate == 10000000);

    CHECK_FALSE(try_parse_header(dir.write("bad.wav", rf64File(1, 12, 1000))).valid);
}

TEST_CASE("try_parse_header detects ZIQ files", "[common][detect_header]")
{
    TempDir dir;
    HeaderInfo info = try_parse_header(dir.write("rec.ziq", ziqFile(3000000)));
    CHECK(info.valid);
    CHECK(info.type == "ziq");
    CHECK(info.samplerate == 3000000);
}

TEST_CASE("try_parse_header on raw data is invalid", "[common][detect_header]")
{
    TempDir dir;
    HeaderInfo info = try_parse_header(dir.write("raw.cf32", std::vector<uint8_t>(256, 0x11)));
    CHECK_FALSE(info.valid);
    CHECK(info.samplerate == 0);
    CHECK(info.type == "");
}

TEST_CASE("try_get_params_from_input_file fills parameters from the header", "[common][detect_header]")
{
    TempDir dir;
    std::string path = dir.write("capture.wav", wavFile(1, 16, 250000));

    nlohmann::json params;
    try_get_params_from_input_file(params, path);
    CHECK(params["samplerate"] == 250000);
    CHECK(params["baseband_format"] == "cs16");
    CHECK_FALSE(params.contains("frequency"));
    CHECK_FALSE(params.contains("start_timestamp"));

    // Existing parameters are not overwritten
    nlohmann::json preset = {{"samplerate", 1}, {"baseband_format", "cf32"}};
    try_get_params_from_input_file(preset, path);
    CHECK(preset["samplerate"] == 1);
    CHECK(preset["baseband_format"] == "cf32");
}

TEST_CASE("try_get_params_from_input_file falls back to filename metadata", "[common][detect_header]")
{
    TempDir dir;
    std::string path = dir.write("2024-01-02_03-04-05_2400000SPS_137100000Hz.cs8", std::vector<uint8_t>(256, 0));

    nlohmann::json params;
    try_get_params_from_input_file(params, path);
    CHECK(params["samplerate"] == 2400000);
    CHECK(params["frequency"] == 137100000);
    CHECK(params["start_timestamp"] == 1704164645);
    CHECK(params["baseband_format"] == "cs8");

    nlohmann::json preset = {{"frequency", 5}, {"start_timestamp", 6}};
    try_get_params_from_input_file(preset, path);
    CHECK(preset["frequency"] == 5);
    CHECK(preset["start_timestamp"] == 6);
}

TEST_CASE("try_get_params_from_input_file ignores missing files and directories", "[common][detect_header]")
{
    TempDir dir;
    nlohmann::json params = nlohmann::json::object();

    try_get_params_from_input_file(params, (dir.path / "does_not_exist.cf32").string());
    try_get_params_from_input_file(params, dir.path.string());

    CHECK(params.empty());
}
