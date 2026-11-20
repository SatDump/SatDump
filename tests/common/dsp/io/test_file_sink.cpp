#include "common/dsp/io/file_sink.h"
#include "common/dsp/io/baseband_interface.h"
#include "../dsp_test_helpers.h"
#include "io_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;
using dsp_test_io::readLE;

namespace
{
    std::vector<complex_t> testSignal(size_t n)
    {
        // Stays within [-1, 1] so integer formats don't clip
        return dsp_test::complexTone(n, 0.05, 0.9);
    }

    // Records the signal with the given format, returns the path written
    std::string record(dsp_test_io::TempPath &tmp, dsp::BasebandType type, const std::vector<complex_t> &sig, uint64_t &written)
    {
        auto input = std::make_shared<dsp::stream<complex_t>>();
        dsp::FileSinkBlock sink(input);
        sink.set_output_sample_type(type);
        std::string path = sink.start_recording(tmp.path(), 48000);
        tmp.with("." + (std::string)type);

        auto r = dsp_test::runBlock(sink, input, sig, 4096);
        REQUIRE(r.completed);
        written = sink.get_written();
        sink.stop_recording();
        CHECK(sink.get_written() == 0); // Counters reset on stop
        return path;
    }

    std::vector<complex_t> readBack(const std::string &path, dsp::BasebandType type, size_t n)
    {
        dsp::BasebandReader rd;
        rd.set_file(path, type);
        std::vector<complex_t> out(n);
        rd.read_samples(out.data(), n);
        rd.close();
        return out;
    }
}

TEST_CASE("FileSinkBlock names the file after the sample format", "[common][dsp][file_sink]")
{
    dsp_test_io::TempPath tmp("sink_name");
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::FileSinkBlock sink(input);
    sink.set_output_sample_type(dsp::CS_16);
    std::string path = sink.start_recording(tmp.path(), 1000);
    tmp.with(".cs16");
    CHECK(path == tmp.path() + ".cs16");
    CHECK(std::filesystem::exists(path));
    sink.stop_recording();

    // override_filename uses the path verbatim
    std::string exact = tmp.with(".raw");
    CHECK(sink.start_recording(exact, 1000, true) == exact);
    CHECK(std::filesystem::exists(exact));
    sink.stop_recording();
}

TEST_CASE("FileSinkBlock records cf32 exactly", "[common][dsp][file_sink]")
{
    dsp_test_io::TempPath tmp("sink_cf32");
    auto sig = testSignal(10000);
    uint64_t written = 0;
    auto path = record(tmp, dsp::CF_32, sig, written);

    CHECK(written == sig.size() * sizeof(complex_t));
    CHECK(std::filesystem::file_size(path) == sig.size() * sizeof(complex_t));
    auto back = readBack(path, dsp::CF_32, sig.size());
    for (size_t i = 0; i < sig.size(); i++)
        REQUIRE(dsp_test::absDiff(back[i], sig[i]) == 0.0f);
}

TEST_CASE("FileSinkBlock integer formats round trip within quantization", "[common][dsp][file_sink]")
{
    struct Case
    {
        dsp::BasebandTypeEnum type;
        size_t bytes_per_sample;
        float tolerance;
    };
    for (Case c : {Case{dsp::CS_32, 8, 1e-6f}, Case{dsp::CS_16, 4, 1.0f / 32767}, Case{dsp::CS_8, 2, 1.0f / 127}})
    {
        dsp::BasebandType type(c.type);
        INFO((std::string)type);
        dsp_test_io::TempPath tmp("sink_int");
        auto sig = testSignal(5000);
        uint64_t written = 0;
        auto path = record(tmp, type, sig, written);

        CHECK(written == sig.size() * c.bytes_per_sample);
        CHECK(std::filesystem::file_size(path) == sig.size() * c.bytes_per_sample);
        auto back = readBack(path, type, sig.size());
        for (size_t i = 0; i < sig.size(); i++)
        {
            REQUIRE(fabs(back[i].real - sig[i].real) <= c.tolerance);
            REQUIRE(fabs(back[i].imag - sig[i].imag) <= c.tolerance);
        }
    }
}

TEST_CASE("FileSinkBlock writes a valid WAV file", "[common][dsp][file_sink]")
{
    dsp_test_io::TempPath tmp("sink_wav");
    auto sig = testSignal(3000);
    uint64_t written = 0;
    auto path = record(tmp, dsp::WAV_16, sig, written);
    CHECK(written == sig.size() * 4);

    auto b = dsp_test_io::readFile(path);
    REQUIRE(b.size() == 44 + sig.size() * 4);
    CHECK(memcmp(&b[0], "RIFF", 4) == 0);
    CHECK(readLE<uint32_t>(b, 24) == 48000);
    CHECK(readLE<uint16_t>(b, 22) == 2);
    CHECK(readLE<uint32_t>(b, 40) == sig.size() * 4);
    CHECK(readLE<uint32_t>(b, 4) == sig.size() * 4 + 36);

    auto back = readBack(path, dsp::WAV_16, sig.size());
    for (size_t i = 0; i < sig.size(); i++)
        REQUIRE(dsp_test::absDiff(back[i], sig[i]) < 2.0f / 32767);
}

TEST_CASE("FileSinkBlock ignores samples when not recording", "[common][dsp][file_sink]")
{
    dsp_test_io::TempPath tmp("sink_idle");
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::FileSinkBlock sink(input);
    std::string path = sink.start_recording(tmp.path(), 1000);
    tmp.with(".cf32");
    sink.stop_recording();

    // Not recording anymore: the samples are consumed but dropped
    auto r = dsp_test::runBlock(sink, input, testSignal(2000), 500);
    REQUIRE(r.completed);
    CHECK(sink.get_written() == 0);
    CHECK(std::filesystem::file_size(path) == 0);
}
