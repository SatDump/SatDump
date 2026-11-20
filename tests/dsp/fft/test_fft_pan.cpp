#include "common/dsp/buffer.h"
#include "common/dsp/window/window.h"
#include "dsp/fft/fft_helper.h"
#include "dsp/fft/fft_pan.h"
#include "../filter/ndsp_sig_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;
using namespace satdump::ndsp;

namespace
{
    struct FFTCapture
    {
        std::mutex mtx;
        std::vector<float> last;
        int count = 0;
    };

    std::shared_ptr<FFTPanBlock> makePan(int size, uint64_t samplerate, int rate, std::shared_ptr<FFTCapture> cap)
    {
        auto pan = std::make_shared<FFTPanBlock>();
        pan->set_fft_settings(size, samplerate, rate);
        REQUIRE(pan->set_cfg("avg_num", 1.0) == Block::RES_OK);
        // The averaging buffer is never initialized by the block (see the report), clear it
        std::fill(pan->output_fft_buff, pan->output_fft_buff + dsp::STREAM_BUFFER_SIZE, 0.0f);
        pan->on_fft = [cap](float *v, size_t n)
        {
            std::lock_guard<std::mutex> lg(cap->mtx);
            cap->last.assign(v, v + n);
            cap->count++;
        };
        return pan;
    }

    ndsp_sig_test::RunOptions sinkOptions(size_t chunk)
    {
        ndsp_sig_test::RunOptions o;
        o.chunk = chunk;
        o.out_indices = {};
        return o;
    }
} // namespace

TEST_CASE("ndsp FFTPanBlock spectrum of a tone peaks in the expected (centered) bin", "[dsp][fft]")
{
    const int N = 1024;
    for (int k : {100, -200, 0, 511})
    {
        INFO("bin " << k);
        auto cap = std::make_shared<FFTCapture>();
        auto pan = makePan(N, N * 8, 8, cap); // One FFT every N samples
        auto in = ndsp_sig_test::complexTone(N * 20, 2 * ndsp_sig_test::kPi * k / N, 1.0);
        auto r = ndsp_sig_test::runBlock<complex_t, float>(pan, in, sinkOptions(4096));
        REQUIRE(r.completed);

        // Each FFT needs strictly more than N buffered samples
        CHECK(cap->count >= 18);
        CHECK(cap->count <= 19);
        REQUIRE(cap->last.size() == N);

        // DC is in the middle of the output
        size_t peak = ndsp_sig_test::argmax(cap->last);
        CHECK(peak == (size_t)((k + N / 2 + N) % N));

        // Power in dB, normalized by the FFT size : coherent gain of the Nuttall window
        double wsum = 0;
        for (int i = 0; i < N; i++)
            wsum += dsp::window::nuttall(i, N - 1);
        CHECK(cap->last[peak] == Approx(20 * log10(wsum / N)).margin(0.2));
        // Far from the tone, the leakage is very low
        CHECK(cap->last[(peak + N / 4) % N] < cap->last[peak] - 80);
    }
}

TEST_CASE("ndsp FFTPanBlock skips samples when the FFT rate is lower than samplerate / size", "[dsp][fft]")
{
    const int N = 512;
    auto cap = std::make_shared<FFTCapture>();
    // One FFT every 4096 samples, of the first 512 of them
    auto pan = makePan(N, 4096 * 10, 10, cap);
    auto in = ndsp_sig_test::complexTone(4096 * 12, 2 * ndsp_sig_test::kPi * 50 / N, 1.0);
    auto r = ndsp_sig_test::runBlock<complex_t, float>(pan, in, sinkOptions(4096));
    REQUIRE(r.completed);
    CHECK(cap->count >= 10);
    CHECK(cap->count <= 11);
    CHECK(ndsp_sig_test::argmax(cap->last) == (size_t)(50 + N / 2));
}

TEST_CASE("ndsp FFTPanBlock configuration", "[dsp][fft]")
{
    FFTPanBlock pan;
    CHECK(pan.d_id == "fft_pan_cc");
    CHECK(pan.get_inputs().size() == 1);
    CHECK(pan.get_outputs().empty());
    CHECK(pan.is_async() == false);

    auto def = ndsp_sig_test::getAll(pan);
    CHECK(def["size"] == 65536);
    CHECK(def["samplerate"] == 10000000);
    CHECK(def["rate"] == 20);
    CHECK(def["avg_num"].get<float>() == 10);
    CHECK(pan.get_cfg_list()["size"]["list"].size() == 5);

    // Set one at a time, so the intermediate settings stay reasonable
    CHECK(pan.set_cfg("samplerate", 2048000) == Block::RES_OK);
    CHECK(pan.set_cfg("size", 8192) == Block::RES_OK);
    CHECK(pan.set_cfg("rate", 30) == Block::RES_OK);
    CHECK(pan.set_cfg("avg_num", 4.0) == Block::RES_OK);
    CHECK(ndsp_sig_test::getAll(pan) == nlohmann::json({{"size", 8192}, {"samplerate", 2048000}, {"rate", 30}, {"avg_num", 4.0}}));
    // Rates below 1 are clamped by set_fft_settings
    pan.set_fft_settings(8192, 2048000, 0);
    CHECK(pan.get_cfg("rate") == 1);
    CHECK_THROWS(pan.set_cfg("x", 1));
    CHECK_THROWS(pan.get_cfg("x"));
}

TEST_CASE("ndsp FFTHelper forward and backward transforms", "[dsp][fft]")
{
    const int N = 64;
    FFTHelper<complex_t> fwd(N, true);
    FFTHelper<complex_t> bwd(N, false);
    for (int i = 0; i < N; i++)
        fwd.fft_in[i] = ndsp_sig_test::fromStd(std::polar(1.0, 2 * ndsp_sig_test::kPi * 5 * i / N));
    fwd.execute();
    for (int i = 0; i < N; i++)
    {
        double m = std::abs(ndsp_sig_test::toStd(fwd.fft_out[i]));
        if (i == 5)
            CHECK(m == Approx(N).epsilon(1e-4));
        else
            CHECK(m < 1e-3);
    }

    // Unnormalized backward transform of the spectrum gives back N times the tone
    for (int i = 0; i < N; i++)
        bwd.fft_in[i] = fwd.fft_out[i];
    bwd.execute();
    for (int i = 0; i < N; i++)
    {
        auto expected = std::polar((double)N, 2 * ndsp_sig_test::kPi * 5 * i / N);
        CHECK(bwd.fft_out[i].real == Approx(expected.real()).margin(1e-2));
        CHECK(bwd.fft_out[i].imag == Approx(expected.imag()).margin(1e-2));
    }
}
