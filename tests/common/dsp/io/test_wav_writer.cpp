// wav_writer.h is not self-contained (uses uint32_t without <cstdint>), so it has to come first.
// It is in its own block to keep clang-format from moving it.
#include <cstdint>

#include "common/dsp/io/io_test_helpers.h"
#include "common/dsp/io/wav_writer.h"
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <fstream>

using dsp_test_io::readLE;

TEST_CASE("WavWriter writes a 44 byte PCM16 header", "[common][dsp][wav_writer]")
{
    dsp_test_io::TempPath tmp("wav_header");
    {
        std::ofstream f(tmp.path(), std::ios::binary);
        dsp::WavWriter w(f);
        w.write_header(48000, 2);
    }

    auto b = dsp_test_io::readBytes(tmp.path());
    REQUIRE(b.size() == 44);
    CHECK(memcmp(&b[0], "RIFF", 4) == 0);
    CHECK(memcmp(&b[8], "WAVEfmt ", 8) == 0);
    CHECK(readLE<uint32_t>(b, 16) == 16);            // fmt chunk size
    CHECK(readLE<uint16_t>(b, 20) == 1);             // PCM
    CHECK(readLE<uint16_t>(b, 22) == 2);             // Channels
    CHECK(readLE<uint32_t>(b, 24) == 48000);         // Samplerate
    CHECK(readLE<uint32_t>(b, 28) == 48000 * 2 * 2); // Byte rate
    CHECK(readLE<uint16_t>(b, 32) == 4);             // Block align
    CHECK(readLE<uint16_t>(b, 34) == 16);            // Bits per sample
    CHECK(memcmp(&b[36], "data", 4) == 0);
}

TEST_CASE("WavWriter finish_header patches the sizes", "[common][dsp][wav_writer]")
{
    dsp_test_io::TempPath tmp("wav_finish");
    const uint32_t data_size = 4000;
    {
        std::ofstream f(tmp.path(), std::ios::binary);
        dsp::WavWriter w(f);
        w.write_header(8000, 1);
        std::vector<char> pcm(data_size, 0x11);
        f.write(pcm.data(), pcm.size());
        w.finish_header(data_size);
    }

    auto b = dsp_test_io::readBytes(tmp.path());
    REQUIRE(b.size() == 44 + data_size);
    CHECK(readLE<uint32_t>(b, 4) == data_size + 36);
    CHECK(readLE<uint32_t>(b, 40) == data_size);
    CHECK(readLE<uint16_t>(b, 22) == 1);
    CHECK(readLE<uint32_t>(b, 28) == 8000 * 2);
    // Payload untouched
    CHECK(b[44] == 0x11);
    CHECK(b.back() == 0x11);
}

TEST_CASE("WavWriter RF64 header layout", "[common][dsp][wav_writer]")
{
    dsp_test_io::TempPath tmp("wav_rf64");
    const uint64_t final_size = 6000000000ull; // > 4 GiB
    {
        std::ofstream f(tmp.path(), std::ios::binary);
        dsp::WavWriter w(f);
        w.write_header(1000000, 2, true, final_size);
    }

    auto b = dsp_test_io::readBytes(tmp.path());
    REQUIRE(b.size() == 80);
    CHECK(memcmp(&b[0], "RF64", 4) == 0);
    CHECK(readLE<uint32_t>(b, 4) == 0xFFFFFFFF);
    CHECK(memcmp(&b[8], "WAVEds64", 8) == 0);
    CHECK(readLE<uint32_t>(b, 16) == 28);
    CHECK(readLE<uint64_t>(b, 20) == final_size + 72); // RIFF size
    CHECK(readLE<uint64_t>(b, 28) == final_size);      // Data size
    CHECK(readLE<uint64_t>(b, 36) == final_size / 4);  // Sample count
    CHECK(memcmp(&b[48], "fmt ", 4) == 0);
    CHECK(readLE<uint32_t>(b, 60) == 1000000);
    CHECK(memcmp(&b[72], "data", 4) == 0);
    CHECK(readLE<uint32_t>(b, 76) == 0xFFFFFFFF);
}
