#include "common/dsp/fft/fft_pan.h"
#include "../dsp_test_helpers.h"
#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <mutex>

using Catch::Approx;

namespace
{
    struct PanCapture
    {
        std::mutex mtx;
        int count = 0;
        std::vector<float> last;
    };

    // Runs a tone through the FFT pan block, returning how many FFTs were produced and the last one
    void runPan(PanCapture &cap, const std::vector<complex_t> &in, int fft_size, uint64_t samplerate, int rate, float avg)
    {
        auto input = std::make_shared<dsp::stream<complex_t>>();
        dsp::FFTPanBlock pan(input);
        pan.set_fft_settings(fft_size, samplerate, rate);
        pan.avg_num = avg;
        pan.on_fft = [&cap, fft_size](float *v)
        {
            std::lock_guard<std::mutex> lck(cap.mtx);
            cap.count++;
            cap.last.assign(v, v + fft_size);
        };
        auto r = dsp_test::runBlock(pan, input, in, 4096);
        REQUIRE(r.completed);
        // The block only reports through the callback
        CHECK(r.output.empty());
    }

    int argmax(const std::vector<float> &v) { return std::max_element(v.begin(), v.end()) - v.begin(); }
}

TEST_CASE("FFTPanBlock peak bin follows the tone frequency, DC centered", "[common][dsp][fft_pan]")
{
    const int N = 1024;
    // samplerate / rate = N: one FFT every N samples, no skipping
    for (int bin : {0, 100, -100, 300, -511})
    {
        INFO("bin = " << bin);
        PanCapture cap;
        runPan(cap, dsp_test::complexTone(8 * N, 2 * M_PI * bin / N), N, N * 60, 60, 1);
        std::lock_guard<std::mutex> lck(cap.mtx);
        REQUIRE(cap.count > 0);
        REQUIRE(cap.last.size() == (size_t)N);
        // Spectrum is shifted so DC lands in the middle
        CHECK(argmax(cap.last) == N / 2 + bin);
    }
}

TEST_CASE("FFTPanBlock output is in dB and rejects off-peak bins", "[common][dsp][fft_pan]")
{
    const int N = 2048;
    PanCapture cap;
    runPan(cap, dsp_test::complexTone(8 * N, 2 * M_PI * 200.0 / N), N, N * 30, 30, 1);
    std::lock_guard<std::mutex> lck(cap.mtx);
    REQUIRE(cap.count > 0);

    int peak = argmax(cap.last);
    CHECK(peak == N / 2 + 200);
    // Nuttall window sidelobes are very low: far bins are > 80 dB down
    for (int i = 0; i < N; i++)
        if (abs(i - peak) > 10)
            REQUIRE(cap.last[i] < cap.last[peak] - 80.0f);

    // 6 dB per doubling of amplitude
    PanCapture cap2;
    runPan(cap2, dsp_test::complexTone(8 * N, 2 * M_PI * 200.0 / N, 2.0), N, N * 30, 30, 1);
    std::lock_guard<std::mutex> lck2(cap2.mtx);
    CHECK(cap2.last[peak] - cap.last[peak] == Approx(20 * log10(2.0)).margin(0.01));
}

TEST_CASE("FFTPanBlock produces one FFT per samplerate / rate samples", "[common][dsp][fft_pan]")
{
    const int N = 512;
    // rbuffer_rate = 40960 / 20 = 2048 samples per FFT, of which only N are transformed
    PanCapture cap;
    runPan(cap, dsp_test::complexTone(2048 * 20 + 100, 0.3), N, 40960, 20, 1);
    std::lock_guard<std::mutex> lck(cap.mtx);
    // An FFT is computed only once strictly more than rbuffer_rate samples are buffered
    CHECK(cap.count >= 19);
    CHECK(cap.count <= 20);
}

TEST_CASE("FFTPanBlock averaging converges to the instantaneous spectrum", "[common][dsp][fft_pan]")
{
    const int N = 256;
    PanCapture fast, slow;
    auto in = dsp_test::complexTone(N * 400, 2 * M_PI * 20.0 / N);
    runPan(fast, in, N, N * 10, 10, 1);
    runPan(slow, in, N, N * 10, 10, 10);
    std::lock_guard<std::mutex> l1(fast.mtx), l2(slow.mtx);
    REQUIRE(fast.count == slow.count);
    // Exponential average from 0 with rate 1/10, after ~400 FFTs it matches
    CHECK(slow.last[N / 2 + 20] == Approx(fast.last[N / 2 + 20]).margin(0.01));
}
