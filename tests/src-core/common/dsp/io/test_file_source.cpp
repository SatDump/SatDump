#include "common/dsp/dsp_test_helpers.h"
#include "common/dsp/io/file_source.h"
#include "common/dsp/io/io_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

namespace
{
    // Runs a file source until it reports EOF, returns everything it output
    std::vector<complex_t> runSource(dsp::FileSourceBlock &src)
    {
        dsp_test::Collector<complex_t> col(src.output_stream);
        src.start();
        auto deadline = std::chrono::steady_clock::now() + dsp_test::kTimeout;
        bool reached = true;
        while (!src.eof())
        {
            if (std::chrono::steady_clock::now() > deadline)
            {
                reached = false;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        src.stop();
        auto out = col.finish();
        REQUIRE(reached);
        return out;
    }

    std::vector<complex_t> ramp(size_t n)
    {
        std::vector<complex_t> v(n);
        for (size_t i = 0; i < n; i++)
            v[i] = complex_t((float)i, 0.5f * i);
        return v;
    }
} // namespace

TEST_CASE("FileSourceBlock streams a cf32 file", "[common][dsp][file_source]")
{
    dsp_test_io::TempPath tmp("src_cf32");
    auto data = ramp(1000);
    dsp_test_io::writeBytes(tmp.path(), data);

    dsp::FileSourceBlock src(tmp.path(), dsp::CF_32, 256);
    CHECK(src.getFilesize() == data.size() * sizeof(complex_t));
    CHECK(src.getPosition() == 0);
    CHECK_FALSE(src.eof());

    auto out = runSource(src);
    REQUIRE(out.size() >= data.size());
    size_t mismatches = 0, first = 0;
    for (size_t i = 0; i < data.size(); i++)
        if ((!(out[i].real == data[i].real) || !(out[i].imag == data[i].imag)) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
    CHECK(src.eof());
}

TEST_CASE("FileSourceBlock iq_swap exchanges I and Q", "[common][dsp][file_source]")
{
    dsp_test_io::TempPath tmp("src_swap");
    auto data = ramp(512);
    dsp_test_io::writeBytes(tmp.path(), data);

    dsp::FileSourceBlock src(tmp.path(), dsp::CF_32, 128, true);
    auto out = runSource(src);
    REQUIRE(out.size() >= data.size());
    for (size_t i = 0; i < data.size(); i++)
    {
        REQUIRE(out[i].real == data[i].imag);
        REQUIRE(out[i].imag == data[i].real);
    }
}

TEST_CASE("FileSourceBlock converts cs16 files", "[common][dsp][file_source]")
{
    dsp_test_io::TempPath tmp("src_cs16");
    std::vector<int16_t> data(2000);
    for (int i = 0; i < 1000; i++)
    {
        data[2 * i] = i * 30;
        data[2 * i + 1] = -i * 30;
    }
    dsp_test_io::writeBytes(tmp.path(), data);

    dsp::FileSourceBlock src(tmp.path(), dsp::CS_16, 250);
    auto out = runSource(src);
    REQUIRE(out.size() >= 1000);
    size_t mismatches = 0, first = 0;
    for (int i = 0; i < 1000; i++)
        if ((!(out[i].real == Approx(i * 30 / 32767.0f).margin(1e-6)) || !(out[i].imag == Approx(-i * 30 / 32767.0f).margin(1e-6))) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}

// Bug: baseband_interface.h:233 - BasebandReader::read_samples always returns buffer_size, even
// when the read hit EOF short (or read nothing). FileSourceBlock then emits a whole extra buffer
// of stale samples at the end of every file.
TEST_CASE("FileSourceBlock outputs exactly the samples in the file", "[common][dsp][file_source][!shouldfail]")
{
    dsp_test_io::TempPath tmp("src_count");
    auto data = ramp(1000);
    dsp_test_io::writeBytes(tmp.path(), data);

    dsp::FileSourceBlock src(tmp.path(), dsp::CF_32, 256);
    auto out = runSource(src);
    CHECK(out.size() == data.size());
}
