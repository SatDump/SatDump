#pragma once

/*
Shared helpers for the common/dsp tests.
Everything lives in the dsp_test namespace and is inline, as every test file
is linked into the same executable.

The streaming harness works like this :
- A Collector thread drains a block's output stream into a vector.
- feedStream() pushes the input in chunks, then pushes two empty buffers.
  Blocks only take a new input buffer once they are done with the previous one
  (output swap included), so once the second empty buffer has been accepted,
  every real output has been posted to the output stream.
- The block is then stopped, and Collector::finish() posts one last empty
  buffer on the (now idle) output stream, which only succeeds once the
  collector consumed everything before it.
Everything blocking is bounded by a timeout so a hung block fails the test.
*/

#include "common/dsp/buffer.h"
#include "common/dsp/complex.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>
#include <volk/volk.h>

namespace dsp_test
{
    constexpr double kPi = 3.14159265358979323846;

    // Generous, as other heavy jobs may run on the machine at the same time
    constexpr std::chrono::milliseconds kTimeout(20000);

    // Deterministic LCG, so failures are reproducible
    struct Lcg
    {
        uint32_t state;
        explicit Lcg(uint32_t seed = 0x12345678) : state(seed) {}
        uint32_t next()
        {
            state = state * 1664525u + 1013904223u;
            return state;
        }
        // Uniform in [0, 1)
        double uniform() { return (next() >> 8) * (1.0 / 16777216.0); }
        // Random +1 / -1
        float sign() { return (next() >> 31) ? 1.0f : -1.0f; }
    };

    // Volk-aligned scratch buffer, for code that expects tree-style allocations
    template <typename T>
    class VolkBuffer
    {
    public:
        explicit VolkBuffer(size_t size) : ptr(dsp::create_volk_buffer<T>(size)), len(size) {}
        ~VolkBuffer() { volk_free(ptr); }
        VolkBuffer(const VolkBuffer &) = delete;
        VolkBuffer &operator=(const VolkBuffer &) = delete;
        T *data() { return ptr; }
        T &operator[](size_t i) { return ptr[i]; }
        size_t size() const { return len; }

    private:
        T *ptr;
        size_t len;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Signal generation / analysis
    ///////////////////////////////////////////////////////////////////////////

    // Complex tone, freq in radians per sample
    inline std::vector<complex_t> complexTone(size_t n, double freq, double amp = 1.0, double phase0 = 0.0)
    {
        std::vector<complex_t> v(n);
        for (size_t i = 0; i < n; i++)
        {
            double p = phase0 + freq * (double)i;
            v[i] = complex_t(amp * cos(p), amp * sin(p));
        }
        return v;
    }

    // Real cosine, freq in radians per sample
    inline std::vector<float> realTone(size_t n, double freq, double amp = 1.0, double phase0 = 0.0)
    {
        std::vector<float> v(n);
        for (size_t i = 0; i < n; i++)
            v[i] = amp * cos(phase0 + freq * (double)i);
        return v;
    }

    // |H(e^jw)| of real taps
    inline double magResponse(const std::vector<float> &taps, double w)
    {
        std::complex<double> acc = 0;
        for (size_t n = 0; n < taps.size(); n++)
            acc += (double)taps[n] * std::polar(1.0, -w * (double)n);
        return std::abs(acc);
    }

    // |H(e^jw)| of complex taps
    inline double magResponse(const std::vector<complex_t> &taps, double w)
    {
        std::complex<double> acc = 0;
        for (size_t n = 0; n < taps.size(); n++)
            acc += std::complex<double>(taps[n].real, taps[n].imag) * std::polar(1.0, -w * (double)n);
        return std::abs(acc);
    }

    inline double toDb(double v) { return 20.0 * log10(v); }

    // Average frequency (rad/sample) of a complex signal over [start, end)
    inline double estimateFreq(const std::vector<complex_t> &x, size_t start, size_t end)
    {
        std::complex<double> acc = 0;
        for (size_t i = start + 1; i < end; i++)
            acc += std::complex<double>(x[i].real, x[i].imag) * std::conj(std::complex<double>(x[i - 1].real, x[i - 1].imag));
        return std::arg(acc);
    }

    // Frequency (rad/sample) of a real tone over [start, end), from zero crossings
    inline double estimateRealFreq(const std::vector<float> &x, size_t start, size_t end)
    {
        int crossings = 0;
        size_t first = 0, last = 0;
        for (size_t i = start + 1; i < end; i++)
        {
            if ((x[i - 1] < 0) != (x[i] < 0))
            {
                if (crossings == 0)
                    first = i;
                last = i;
                crossings++;
            }
        }
        if (crossings < 2)
            return 0;
        return kPi * (crossings - 1) / (double)(last - first);
    }

    template <typename T>
    inline double rms(const std::vector<T> &x, size_t start, size_t end)
    {
        double acc = 0;
        for (size_t i = start; i < end; i++)
        {
            if constexpr (std::is_same_v<T, complex_t>)
                acc += x[i].real * x[i].real + x[i].imag * x[i].imag;
            else
                acc += (double)x[i] * (double)x[i];
        }
        return sqrt(acc / (double)(end - start));
    }

    template <typename T>
    inline double rms(const std::vector<T> &x) { return rms(x, 0, x.size()); }

    // Causal convolution y[n] = sum_k taps[k] x[n-k], zero initial history, same length as x
    template <typename T>
    inline std::vector<T> referenceFir(const std::vector<T> &x, const std::vector<float> &taps)
    {
        std::vector<T> y(x.size());
        for (size_t n = 0; n < x.size(); n++)
        {
            double re = 0, im = 0;
            for (size_t k = 0; k < taps.size() && k <= n; k++)
            {
                if constexpr (std::is_same_v<T, complex_t>)
                {
                    re += (double)taps[k] * x[n - k].real;
                    im += (double)taps[k] * x[n - k].imag;
                }
                else
                    re += (double)taps[k] * x[n - k];
            }
            if constexpr (std::is_same_v<T, complex_t>)
                y[n] = complex_t(re, im);
            else
                y[n] = re;
        }
        return y;
    }

    inline float absDiff(float a, float b) { return std::fabs(a - b); }
    inline float absDiff(complex_t a, complex_t b) { return std::hypot(a.real - b.real, a.imag - b.imag); }

    ///////////////////////////////////////////////////////////////////////////
    // Streaming harness
    ///////////////////////////////////////////////////////////////////////////

    // Drains a stream into a vector on its own thread
    template <typename T>
    class Collector
    {
    public:
        explicit Collector(std::shared_ptr<dsp::stream<T>> s) : stream(s)
        {
            thread = std::thread([this]()
                                 {
                                     while (true)
                                     {
                                         int n = stream->read();
                                         if (n < 0)
                                             break;
                                         {
                                             std::lock_guard<std::mutex> lck(mtx);
                                             data.insert(data.end(), stream->readBuf, stream->readBuf + n);
                                         }
                                         stream->flush();
                                     } });
        }

        ~Collector()
        {
            if (thread.joinable())
                finish();
        }

        Collector(const Collector &) = delete;
        Collector &operator=(const Collector &) = delete;

        size_t size()
        {
            std::lock_guard<std::mutex> lck(mtx);
            return data.size();
        }

        // Waits (bounded) until at least n samples were collected
        bool waitFor(size_t n, std::chrono::milliseconds timeout = kTimeout)
        {
            auto deadline = std::chrono::steady_clock::now() + timeout;
            while (size() < n)
            {
                if (std::chrono::steady_clock::now() > deadline)
                    return false;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            return true;
        }

        // Must only be called once whatever writes to the stream is idle (stopped)
        std::vector<T> finish()
        {
            if (thread.joinable())
            {
                // Posting an empty buffer only succeeds once all previous data was consumed
                stream->clearWriteStop();
                stream->swap(0);
                stream->stopReader();
                thread.join();
            }
            std::lock_guard<std::mutex> lck(mtx);
            return data;
        }

    private:
        std::shared_ptr<dsp::stream<T>> stream;
        std::thread thread;
        std::mutex mtx;
        std::vector<T> data;
    };

    // Pushes data into a stream in chunks, then two empty sync buffers. False on timeout.
    template <typename T>
    inline bool feedStream(std::shared_ptr<dsp::stream<T>> s, const std::vector<T> &data, int chunk, std::chrono::milliseconds timeout = kTimeout)
    {
        std::promise<void> done;
        std::future<void> fut = done.get_future();
        std::atomic<bool> aborted{false};

        std::thread feeder([&]()
                           {
                               size_t pos = 0;
                               bool ok = true;
                               while (ok && pos < data.size() && !aborted)
                               {
                                   int n = (int)std::min<size_t>(chunk, data.size() - pos);
                                   memcpy((void *)s->writeBuf, (const void *)&data[pos], n * sizeof(T));
                                   ok = s->swap(n);
                                   pos += n;
                               }
                               for (int i = 0; ok && i < 2 && !aborted; i++)
                                   ok = s->swap(0);
                               done.set_value(); });

        bool ok = fut.wait_for(timeout) == std::future_status::ready;
        if (!ok)
        {
            aborted = true;
            s->stopWriter(); // Unblocks the feeder
        }
        feeder.join();
        return ok;
    }

    template <typename T>
    struct RunResult
    {
        bool completed = false;
        std::vector<T> output;
    };

    template <typename BlockT>
    using BlockOutput = std::remove_reference_t<decltype(std::declval<BlockT &>().output_stream->writeBuf[0])>;

    /*
    Runs a whole block lifecycle : start, feed all the input, stop, collect.
    min_out can be used for blocks with internal stages (hier blocks), where
    the feeding sync alone does not prove the last stage is done.
    */
    template <typename BlockT, typename IN>
    inline RunResult<BlockOutput<BlockT>> runBlock(BlockT &blk, std::shared_ptr<dsp::stream<IN>> input, const std::vector<IN> &data, int chunk = 4096, size_t min_out = 0)
    {
        using OUT = BlockOutput<BlockT>;
        RunResult<OUT> r;
        Collector<OUT> col(blk.output_stream);
        blk.start();
        r.completed = feedStream(input, data, chunk);
        if (r.completed && min_out > 0)
            r.completed = col.waitFor(min_out);
        blk.stop();
        r.output = col.finish();
        return r;
    }
} // namespace dsp_test
