#include "common/dsp/io/baseband_interface.h"
#include "common/dsp/io/io_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fstream>

using Catch::Approx;

namespace
{
    std::vector<complex_t> readAll(dsp::BasebandReader &rd, int n)
    {
        std::vector<complex_t> out(n);
        int got = rd.read_samples(out.data(), n);
        REQUIRE(got == n);
        return out;
    }
} // namespace

TEST_CASE("BasebandReader reads cf32 files as-is", "[common][dsp][baseband_reader]")
{
    dsp_test_io::TempPath tmp("rd_cf32");
    std::vector<complex_t> data(1000);
    for (size_t i = 0; i < data.size(); i++)
        data[i] = complex_t(i * 0.001f, -(float)i);
    dsp_test_io::writeBytes(tmp.path(), data);

    dsp::BasebandReader rd;
    rd.set_file(tmp.path(), dsp::CF_32);
    CHECK(rd.filesize == data.size() * sizeof(complex_t));
    CHECK(rd.progress == 0);

    auto a = readAll(rd, 400);
    for (int i = 0; i < 400; i++)
    {
        REQUIRE(a[i].real == data[i].real);
        REQUIRE(a[i].imag == data[i].imag);
    }
    CHECK(rd.progress == 400 * sizeof(complex_t));
    CHECK_FALSE(rd.is_eof());

    auto b = readAll(rd, 600);
    CHECK(b[599].imag == data[999].imag);
    rd.close();
}

TEST_CASE("BasebandReader converts integer formats to [-1, 1] floats", "[common][dsp][baseband_reader]")
{
    SECTION("cs16")
    {
        dsp_test_io::TempPath tmp("rd_cs16");
        dsp_test_io::writeBytes<int16_t>(tmp.path(), {32767, -32767, 0, 16384, -16384, 1});
        dsp::BasebandReader rd;
        rd.set_file(tmp.path(), dsp::CS_16);
        auto v = readAll(rd, 3);
        CHECK(v[0].real == Approx(1.0f));
        CHECK(v[0].imag == Approx(-1.0f));
        CHECK(v[1].real == 0.0f);
        CHECK(v[1].imag == Approx(0.5f).epsilon(1e-4));
        CHECK(v[2].real == Approx(-0.5f).epsilon(1e-4));
        CHECK(v[2].imag == Approx(1.0f / 32767));
        rd.close();
    }

    SECTION("cs8")
    {
        dsp_test_io::TempPath tmp("rd_cs8");
        dsp_test_io::writeBytes<int8_t>(tmp.path(), {127, -127, 0, 64});
        dsp::BasebandReader rd;
        rd.set_file(tmp.path(), dsp::CS_8);
        auto v = readAll(rd, 2);
        CHECK(v[0].real == Approx(1.0f));
        CHECK(v[0].imag == Approx(-1.0f));
        CHECK(v[1].real == 0.0f);
        CHECK(v[1].imag == Approx(64.0f / 127));
        rd.close();
    }

    SECTION("cu8")
    {
        dsp_test_io::TempPath tmp("rd_cu8");
        // Offset binary centered at 127
        dsp_test_io::writeBytes<uint8_t>(tmp.path(), {254, 0, 127, 127});
        dsp::BasebandReader rd;
        rd.set_file(tmp.path(), dsp::CU_8);
        auto v = readAll(rd, 2);
        CHECK(v[0].real == Approx(1.0f));
        CHECK(v[0].imag == Approx(-1.0f));
        CHECK(v[1].real == 0.0f);
        CHECK(v[1].imag == 0.0f);
        rd.close();
    }

    SECTION("cs32")
    {
        dsp_test_io::TempPath tmp("rd_cs32");
        dsp_test_io::writeBytes<int32_t>(tmp.path(), {2147483647, -2147483647, 1073741824, 0});
        dsp::BasebandReader rd;
        rd.set_file(tmp.path(), dsp::CS_32);
        auto v = readAll(rd, 2);
        CHECK(v[0].real == Approx(1.0f));
        CHECK(v[0].imag == Approx(-1.0f));
        CHECK(v[1].real == Approx(0.5f));
        CHECK(v[1].imag == 0.0f);
        rd.close();
    }
}

TEST_CASE("BasebandReader skips the WAV header", "[common][dsp][baseband_reader]")
{
    dsp_test_io::TempPath tmp("rd_wav");
    {
        std::ofstream f(tmp.path(), std::ios::binary);
        dsp::WavWriter w(f);
        w.write_header(48000, 2);
        int16_t pcm[4] = {32767, 0, -16384, 16384};
        f.write((char *)pcm, sizeof(pcm));
        w.finish_header(sizeof(pcm));
    }

    dsp::BasebandReader rd;
    rd.set_file(tmp.path(), dsp::WAV_16);
    auto v = readAll(rd, 2);
    CHECK(v[0].real == Approx(1.0f));
    CHECK(v[0].imag == 0.0f);
    CHECK(v[1].real == Approx(-0.5f).epsilon(1e-4));
    CHECK(v[1].imag == Approx(0.5f).epsilon(1e-4));
    rd.close();
}

TEST_CASE("BasebandReader set_progress seeks to a percentage of the file", "[common][dsp][baseband_reader]")
{
    dsp_test_io::TempPath tmp("rd_seek");
    std::vector<int16_t> data(2 * 1001);
    for (int i = 0; i < 1001; i++)
    {
        data[2 * i] = i;
        data[2 * i + 1] = -i;
    }
    dsp_test_io::writeBytes(tmp.path(), data);

    dsp::BasebandReader rd;
    rd.set_file(tmp.path(), dsp::CS_16);
    // position = (filesize / samplesize - 1) * progress / 100 = 1000 * 0.5
    rd.set_progress(50);
    auto v = readAll(rd, 1);
    CHECK(v[0].real * 32767 == Approx(500.0f).margin(0.01));
    CHECK(v[0].imag * 32767 == Approx(-500.0f).margin(0.01));

    rd.set_progress(0);
    v = readAll(rd, 1);
    CHECK(v[0].real == 0.0f);
    rd.close();
}

TEST_CASE("BasebandReader should_repeat loops back to the start", "[common][dsp][baseband_reader]")
{
    dsp_test_io::TempPath tmp("rd_repeat");
    std::vector<complex_t> data = {complex_t(1, 2), complex_t(3, 4), complex_t(5, 6)};
    dsp_test_io::writeBytes(tmp.path(), data);

    dsp::BasebandReader rd;
    rd.set_file(tmp.path(), dsp::CF_32);
    rd.should_repeat = true;

    std::vector<complex_t> buf(3);
    rd.read_samples(buf.data(), 3);
    rd.read_samples(buf.data(), 1); // Hits EOF
    CHECK(rd.is_eof());
    // Next read rewinds
    rd.read_samples(buf.data(), 2);
    CHECK(buf[0].real == 1.0f);
    CHECK(buf[1].imag == 4.0f);
    rd.close();
}

TEST_CASE("BasebandReader is_eof is false when nothing is open", "[common][dsp][baseband_reader]")
{
    dsp::BasebandReader rd;
    CHECK_FALSE(rd.is_eof());
}

// Bug: baseband_interface.h:229 - progress is set from input_file.tellg(), which returns -1 once
// a read hits EOF, so progress jumps to UINT64_MAX instead of staying at the file size.
TEST_CASE("BasebandReader progress stays within the file at EOF", "[common][dsp][baseband_reader][!shouldfail]")
{
    dsp_test_io::TempPath tmp("rd_eof");
    std::vector<complex_t> data(100, complex_t(1, 1));
    dsp_test_io::writeBytes(tmp.path(), data);

    dsp::BasebandReader rd;
    rd.set_file(tmp.path(), dsp::CF_32);
    std::vector<complex_t> buf(64);
    rd.read_samples(buf.data(), 64);
    rd.read_samples(buf.data(), 64);
    CHECK(rd.is_eof());
    CHECK(rd.progress <= rd.filesize);
    rd.close();
}
