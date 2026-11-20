#include "common/wav.h"
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>

namespace
{
    std::filesystem::path uniqueTempPath(const std::string &name)
    {
        std::mt19937 rng(std::random_device{}());
        return std::filesystem::temp_directory_path() / ("satdump_test_wav_" + std::to_string(rng()) + "_" + name);
    }

    struct TempFile
    {
        std::filesystem::path path;
        TempFile(const std::string &name, const void *data, size_t size) : path(uniqueTempPath(name))
        {
            std::ofstream f(path, std::ios::binary);
            f.write((const char *)data, size);
        }
        ~TempFile() { std::filesystem::remove(path); }
    };

    wav::WavHeader makeWavHeader(uint16_t format, uint16_t bits, uint32_t samplerate)
    {
        wav::WavHeader h;
        memset(&h, 0, sizeof(h));
        memcpy(h.RIFF, "RIFF", 4);
        memcpy(h.WAVE, "WAVE", 4);
        memcpy(h.fmt, "fmt ", 4);
        memcpy(h.data, "data", 4);
        h.chunk_size = 36;
        h.sub_chunk_size = 16;
        h.audio_Format = format;
        h.channel_cnt = 2;
        h.samplerate = samplerate;
        h.bits_per_sample = bits;
        h.block_align = 2 * bits / 8;
        h.bytes_per_sec = samplerate * h.block_align;
        return h;
    }

    time_t utc(int year, int month, int day, int hour, int minute, int second)
    {
        std::tm t = {};
        t.tm_year = year - 1900;
        t.tm_mon = month - 1;
        t.tm_mday = day;
        t.tm_hour = hour;
        t.tm_min = minute;
        t.tm_sec = second;
        return timegm(&t);
    }

    time_t local(int year, int month, int day, int hour, int minute, int second)
    {
        std::tm t = {};
        t.tm_year = year - 1900;
        t.tm_mon = month - 1;
        t.tm_mday = day;
        t.tm_hour = hour;
        t.tm_min = minute;
        t.tm_sec = second;
        t.tm_isdst = -1;
        return mktime(&t);
    }

    const time_t T_2024_01_02_030405 = 1704164645; // 2024-01-02 03:04:05 UTC
} // namespace

TEST_CASE("wav header structs are packed to their on-disk sizes", "[common][wav]")
{
    CHECK(sizeof(wav::WavHeader) == 44);
    CHECK(sizeof(wav::RF64Header) == 80);
}

TEST_CASE("wav::parseHeaderFromFileWav round-trips a written header", "[common][wav]")
{
    wav::WavHeader h = makeWavHeader(3, 32, 2400000);
    TempFile f("hdr.wav", &h, sizeof(h));

    wav::WavHeader r = wav::parseHeaderFromFileWav(f.path.string());

    CHECK(wav::isValidWav(r));
    CHECK_FALSE(wav::isValidRF64(r));
    CHECK(r.audio_Format == 3);
    CHECK(r.bits_per_sample == 32);
    CHECK(r.samplerate == 2400000);
    CHECK(r.channel_cnt == 2);
    CHECK(r.block_align == 8);
    CHECK(r.bytes_per_sec == 2400000u * 8);
    CHECK(memcmp(r.WAVE, "WAVE", 4) == 0);
}

TEST_CASE("wav::parseHeaderFromFileWav on a missing file returns a zeroed header", "[common][wav]")
{
    wav::WavHeader r = wav::parseHeaderFromFileWav(uniqueTempPath("missing.wav").string());
    CHECK_FALSE(wav::isValidWav(r));
    CHECK_FALSE(wav::isValidRF64(r));
    CHECK(r.samplerate == 0);
    CHECK(r.bits_per_sample == 0);
}

TEST_CASE("wav::parseHeaderFromFileRF64 reads the RF64 layout", "[common][wav]")
{
    wav::RF64Header h;
    memset(&h, 0, sizeof(h));
    memcpy(h.RIFF, "RF64", 4);
    memcpy(h.WAVE, "WAVE", 4);
    memcpy(h.fmt, "fmt ", 4);
    h.audio_Format = 1;
    h.bits_per_sample = 16;
    h.samplerate = 6000000;
    TempFile f("hdr_rf64.wav", &h, sizeof(h));

    wav::RF64Header r = wav::parseHeaderFromFileRF64(f.path.string());
    CHECK(r.samplerate == 6000000);
    CHECK(r.bits_per_sample == 16);
    CHECK(r.audio_Format == 1);

    // RF64 detection is done on the plain WAV header read
    wav::WavHeader w = wav::parseHeaderFromFileWav(f.path.string());
    CHECK(wav::isValidRF64(w));
    CHECK_FALSE(wav::isValidWav(w));
}

TEST_CASE("wav::tryParseFilenameMetadata parses SatDump baseband names", "[common][wav]")
{
    SECTION("complex formats")
    {
        for (std::string ext : {"cf32", "cs32", "cs16", "cs8"})
        {
            wav::FileMetadata md = wav::tryParseFilenameMetadata("/some/dir/2024-01-02_03-04-05_2400000SPS_137100000Hz." + ext);
            INFO(ext);
            CHECK(md.timestamp == T_2024_01_02_030405);
            CHECK(md.samplerate == 2400000);
            CHECK(md.frequency == 137100000);
            CHECK(md.baseband_format == ext);
        }
    }

    SECTION("real formats map to their complex counterparts")
    {
        CHECK(wav::tryParseFilenameMetadata("2024-01-02_03-04-05_2400000SPS_137100000Hz.f32").baseband_format == "cf32");
        CHECK(wav::tryParseFilenameMetadata("2024-01-02_03-04-05_2400000SPS_137100000Hz.s16").baseband_format == "cs16");
        CHECK(wav::tryParseFilenameMetadata("2024-01-02_03-04-05_2400000SPS_137100000Hz.s8").baseband_format == "cs8");
    }

    SECTION("wav files do not carry samplerate or format from the name")
    {
        wav::FileMetadata md = wav::tryParseFilenameMetadata("2024-01-02_03-04-05_2400000SPS_137100000Hz.wav");
        CHECK(md.timestamp == T_2024_01_02_030405);
        CHECK(md.frequency == 137100000);
        CHECK(md.samplerate == 0);
        CHECK(md.baseband_format == "");
    }

    SECTION("with milliseconds")
    {
        wav::FileMetadata md = wav::tryParseFilenameMetadata("2024-01-02_03-04-05-678_2400000SPS_137100000Hz.cs16");
        CHECK(md.timestamp == T_2024_01_02_030405);
        CHECK(md.samplerate == 2400000);
        CHECK(md.frequency == 137100000);
        CHECK(md.baseband_format == "cs16");
    }
}

// BUG: wav.cpp:309 - the millisecond-precision branch omits ".cs32" (and ".s32"),
// which the plain SatDump branch accepts, so the format is lost.
TEST_CASE("wav::tryParseFilenameMetadata handles cs32 names with milliseconds", "[common][wav][!shouldfail]")
{
    wav::FileMetadata md = wav::tryParseFilenameMetadata("2024-01-02_03-04-05-678_2400000SPS_137100000Hz.cs32");
    CHECK(md.samplerate == 2400000);
    CHECK(md.baseband_format == "cs32");
}

TEST_CASE("wav::tryParseFilenameMetadata parses third-party baseband names", "[common][wav]")
{
    SECTION("SDR++ baseband, local time")
    {
        wav::FileMetadata md = wav::tryParseFilenameMetadata("baseband_137100000Hz_12-34-56_02-01-2024.wav");
        CHECK(md.frequency == 137100000);
        CHECK(md.timestamp == local(2024, 1, 2, 12, 34, 56));
    }

    SECTION("SDR Console, local time")
    {
        wav::FileMetadata md = wav::tryParseFilenameMetadata("02-Mar-2024 123456.789 137.500MHz.wav");
        CHECK(md.frequency == 137500000);
        CHECK(md.timestamp == local(2024, 3, 2, 12, 34, 56));
    }

    SECTION("unknown names give empty metadata")
    {
        wav::FileMetadata md = wav::tryParseFilenameMetadata("recording.raw");
        CHECK(md.frequency == 0);
        CHECK(md.samplerate == 0);
        CHECK(md.timestamp == 0);
        CHECK(md.baseband_format == "");
    }
}

TEST_CASE("wav::tryParseFilenameMetadata parses audio file names", "[common][wav]")
{
    SECTION("SDR++ audio, local time")
    {
        wav::FileMetadata md = wav::tryParseFilenameMetadata("audio_137100000Hz_12-34-56_02-01-2024.wav", true);
        CHECK(md.frequency == 137100000);
        CHECK(md.timestamp == local(2024, 1, 2, 12, 34, 56));
    }

    SECTION("HDSDR, frequency in kHz")
    {
        wav::FileMetadata md = wav::tryParseFilenameMetadata("HDSDR_20240102_030405Z_137100kHz_AF.wav", true);
        CHECK(md.frequency == 137100000);
        CHECK(md.timestamp == T_2024_01_02_030405);
    }

    SECTION("SDRSharp")
    {
        wav::FileMetadata md = wav::tryParseFilenameMetadata("SDRSharp_20240102_030405Z_137100000Hz_AF.wav", true);
        CHECK(md.frequency == 137100000);
        CHECK(md.timestamp == T_2024_01_02_030405);
    }

    SECTION("SDRUno, local time")
    {
        wav::FileMetadata md = wav::tryParseFilenameMetadata("SDRUno_20240102_030405_137100000Hz.wav", true);
        CHECK(md.frequency == 137100000);
        CHECK(md.timestamp == local(2024, 1, 2, 3, 4, 5));
    }

    SECTION("AltiWx")
    {
        wav::FileMetadata md = wav::tryParseFilenameMetadata("20240102T030405Z.wav", true);
        CHECK(md.frequency == 0);
        CHECK(md.timestamp == T_2024_01_02_030405);
    }

    SECTION("GQRX")
    {
        wav::FileMetadata md = wav::tryParseFilenameMetadata("gqrx_20240102_030405_137100000.wav", true);
        CHECK(md.frequency == 137100000);
        CHECK(md.timestamp == T_2024_01_02_030405);
    }

    SECTION("WXtoImg")
    {
        wav::FileMetadata md = wav::tryParseFilenameMetadata("20240102030405.wav", true);
        CHECK(md.timestamp == T_2024_01_02_030405);
    }

    SECTION("NOAA APT variants, local time")
    {
        time_t expected = local(2024, 1, 2, 3, 4, 5);
        CHECK(wav::tryParseFilenameMetadata("NOAA1920240102-030405.wav", true).timestamp == expected);
        CHECK(wav::tryParseFilenameMetadata("NOAA19-20240102-030405.wav", true).timestamp == expected);
        CHECK(wav::tryParseFilenameMetadata("NOAA-19-20240102-030405.wav", true).timestamp == expected);
    }

    SECTION("audio names are not parsed in baseband mode and vice versa")
    {
        CHECK(wav::tryParseFilenameMetadata("HDSDR_20240102_030405Z_137100kHz_AF.wav", false).frequency == 0);
        CHECK(wav::tryParseFilenameMetadata("2024-01-02_03-04-05_2400000SPS_137100000Hz.cs16", true).frequency == 0);
    }

    CHECK(utc(2024, 1, 2, 3, 4, 5) == T_2024_01_02_030405);
}
