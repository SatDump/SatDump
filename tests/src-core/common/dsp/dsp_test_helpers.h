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
#include "helpers/random.h"
#include "helpers/signal.h"
#include "helpers/wait.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
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
    using test_helpers::kTimeout;
    using test_helpers::Lcg;

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

    using test_helpers::complexTone;
    using test_helpers::estimateFreq;
    using test_helpers::estimateRealFreq;
    using test_helpers::magResponse;
    using test_helpers::realTone;
    using test_helpers::toDb;

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
    inline double rms(const std::vector<T> &x)
    {
        return rms(x, 0, x.size());
    }

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

    using test_helpers::absDiff;

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
            thread = std::thread(
                [this]()
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
                    }
                });
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

        std::thread feeder(
            [&]()
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
                done.set_value();
            });

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
