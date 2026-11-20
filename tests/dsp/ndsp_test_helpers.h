#pragma once

/*
Shared helpers for the tests of the new DSP framework (src-core/dsp).
Everything lives in the ndsp_test namespace and is inline, as every test file
is linked into the same executable.

The streaming harness works like this :
- A Collector thread drains one block output (a DSPStream) into a vector,
  recording buffer sizes and terminators. It polls with try_dequeue so it
  can always be stopped.
- A Feeder thread pushes the input vector into a DSPStream in fixed-size
  chunks, then pushes a terminator.
- run() wires the block up, starts everything, waits for the feeders, then
  stops (joins) the block and finally drains and stops the collectors.
Every blocking wait is bounded by a timeout. If something hangs, the test
fails and the stuck block / threads are intentionally leaked (never
destroyed, threads detached) so that the rest of the suite can still run.
*/

#include "common/dsp/complex.h"
#include "dsp/base/dsp_buffer.h"
#include "dsp/base/stream.h"
#include "dsp/block.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

namespace ndsp_test
{
    using satdump::ndsp::Block;
    using satdump::ndsp::BlockIO;
    using satdump::ndsp::DSPBuffer;
    using satdump::ndsp::DSPStream;

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
        // Uniform in [-1, 1)
        float symmetric() { return (float)(uniform() * 2.0 - 1.0); }
        // Random +1 / -1
        float sign() { return (next() >> 31) ? 1.0f : -1.0f; }
        // Standard normal (Box-Muller)
        double gauss()
        {
            double u1 = std::max(uniform(), 1e-12), u2 = uniform();
            return sqrt(-2.0 * log(u1)) * cos(2.0 * kPi * u2);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Signal generation / analysis
    ///////////////////////////////////////////////////////////////////////////

    inline std::vector<float> randomFloats(size_t n, uint32_t seed = 1, float scale = 1.0f)
    {
        Lcg rng(seed);
        std::vector<float> v(n);
        for (auto &x : v)
            x = rng.symmetric() * scale;
        return v;
    }

    inline std::vector<complex_t> randomComplex(size_t n, uint32_t seed = 1, float scale = 1.0f)
    {
        Lcg rng(seed);
        std::vector<complex_t> v(n);
        for (auto &x : v)
        {
            float re = rng.symmetric() * scale;
            float im = rng.symmetric() * scale;
            x = complex_t(re, im);
        }
        return v;
    }

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

    // Average frequency (rad/sample) of a complex signal over [start, end)
    inline double estimateFreq(const std::vector<complex_t> &x, size_t start, size_t end)
    {
        std::complex<double> acc = 0;
        for (size_t i = start + 1; i < end; i++)
            acc += std::complex<double>(x[i].real, x[i].imag) * std::conj(std::complex<double>(x[i - 1].real, x[i - 1].imag));
        return std::arg(acc);
    }

    inline float absDiff(float a, float b) { return std::fabs(a - b); }
    inline float absDiff(complex_t a, complex_t b) { return std::hypot(a.real - b.real, a.imag - b.imag); }
    inline float absDiff(int8_t a, int8_t b) { return std::fabs((float)a - (float)b); }
    inline float absDiff(int16_t a, int16_t b) { return std::fabs((float)a - (float)b); }
    inline float absDiff(uint8_t a, uint8_t b) { return std::fabs((float)a - (float)b); }

    inline bool sameBits(float a, float b) { return memcmp(&a, &b, sizeof(float)) == 0; }
    inline bool sameBits(complex_t a, complex_t b) { return sameBits(a.real, b.real) && sameBits(a.imag, b.imag); }

    // Largest absolute difference between two equally sized vectors (inf if sizes differ)
    template <typename T>
    inline double maxAbsDiff(const std::vector<T> &a, const std::vector<T> &b)
    {
        if (a.size() != b.size())
            return INFINITY;
        double m = 0;
        for (size_t i = 0; i < a.size(); i++)
            m = std::max<double>(m, absDiff(a[i], b[i]));
        return m;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Temporary files
    ///////////////////////////////////////////////////////////////////////////

    // Unique temporary path, removed (if it exists) on destruction
    class TempFile
    {
    public:
        explicit TempFile(const std::string &suffix = ".bin")
        {
            // Unique across threads and concurrently running test processes
            static std::atomic<int> counter{0};
            static const unsigned int process_tag = std::random_device()();
            auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
            std::string name = "satdump_ndsp_test_" + std::to_string(process_tag) + "_" + std::to_string((long long)tick) + "_" + std::to_string(counter++) + suffix;
            p = std::filesystem::temp_directory_path() / name;
        }
        ~TempFile()
        {
            std::error_code ec;
            std::filesystem::remove(p, ec);
        }
        TempFile(const TempFile &) = delete;
        TempFile &operator=(const TempFile &) = delete;
        std::string path() const { return p.string(); }

    private:
        std::filesystem::path p;
    };

    template <typename T>
    inline void writeFile(const std::string &path, const std::vector<T> &data)
    {
        std::ofstream f(path, std::ios::binary);
        f.write((const char *)data.data(), data.size() * sizeof(T));
    }

    template <typename T>
    inline std::vector<T> readFile(const std::string &path)
    {
        std::ifstream f(path, std::ios::binary);
        std::vector<char> raw((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        std::vector<T> out(raw.size() / sizeof(T));
        if (!out.empty())
            memcpy((void *)out.data(), raw.data(), out.size() * sizeof(T));
        return out;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Streaming harness
    ///////////////////////////////////////////////////////////////////////////

    // Keeps something alive forever. Used for blocks / streams of a hung test, as
    // destroying a block whose thread is still running throws (and terminates)
    template <typename T>
    inline void leakForever(std::shared_ptr<T> p)
    {
        new std::shared_ptr<T>(p);
    }

    // Polls a predicate until it is true, bounded by a timeout
    inline bool waitUntil(const std::function<bool()> &pred, std::chrono::milliseconds timeout = kTimeout)
    {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        while (!pred())
        {
            if (std::chrono::steady_clock::now() > deadline)
                return false;
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
        return true;
    }

    // Copies samples into a DSPStream buffer and enqueues it. May block on allocation.
    template <typename T>
    inline void pushSamples(DSPStream &s, const T *data, size_t n)
    {
        DSPBuffer b = s.newBufferSamples(n, sizeof(T));
        if (n > 0)
            memcpy((void *)b.getSamples<T>(), (const void *)data, n * sizeof(T));
        b.size = n;
        s.wait_enqueue(b);
    }

    template <typename T>
    inline void pushSamples(DSPStream &s, const std::vector<T> &data)
    {
        pushSamples<T>(s, data.data(), data.size());
    }

    inline void pushTerminator(DSPStream &s, bool propagate = true) { s.wait_enqueue(s.newBufferTerminator(propagate)); }

    // Drains a stream into a vector on its own thread
    template <typename T>
    class Collector
    {
    private:
        struct State
        {
            std::shared_ptr<DSPStream> stream;
            std::mutex mtx;
            std::vector<T> data;
            std::vector<uint32_t> sizes;
            std::vector<uint32_t> max_sizes;
            int terminators = 0;
            int propagating_terminators = 0;
            int buffers_after_terminator = 0;
            std::atomic<bool> stop{false};
            std::atomic<bool> got_terminator{false};
        };

        std::shared_ptr<State> st;
        std::thread th;

    public:
        explicit Collector(std::shared_ptr<DSPStream> s) : st(std::make_shared<State>())
        {
            st->stream = s;
            std::shared_ptr<State> lst = st;
            th = std::thread(
                [lst]()
                {
                    while (true)
                    {
                        DSPBuffer b;
                        if (lst->stream->try_dequeue(b))
                        {
                            bool term = b.isTerminator();
                            {
                                std::lock_guard<std::mutex> lck(lst->mtx);
                                if (term)
                                {
                                    lst->terminators++;
                                    if (b.terminatorShouldPropagate())
                                        lst->propagating_terminators++;
                                }
                                else
                                {
                                    if (lst->got_terminator)
                                        lst->buffers_after_terminator++;
                                    T *p = b.getSamples<T>();
                                    lst->data.insert(lst->data.end(), p, p + b.size);
                                    lst->sizes.push_back(b.size);
                                    lst->max_sizes.push_back(b.max_size);
                                }
                            }
                            lst->stream->free(b);
                            // Only flagged once the buffer was given back, so a test
                            // waiting on this knows the collector is idle
                            if (term)
                                lst->got_terminator = true;
                        }
                        else
                        {
                            // Only exit once the stream is empty
                            if (lst->stop)
                                break;
                            std::this_thread::sleep_for(std::chrono::microseconds(50));
                        }
                    }
                });
        }

        ~Collector()
        {
            if (th.joinable())
                finish();
        }

        Collector(const Collector &) = delete;
        Collector &operator=(const Collector &) = delete;

        size_t size()
        {
            std::lock_guard<std::mutex> lck(st->mtx);
            return st->data.size();
        }

        bool gotTerminator() { return st->got_terminator; }

        // Waits (bounded) until at least n samples were collected
        bool waitFor(size_t n, std::chrono::milliseconds timeout = kTimeout)
        {
            auto deadline = std::chrono::steady_clock::now() + timeout;
            while (size() < n)
            {
                if (std::chrono::steady_clock::now() > deadline)
                    return false;
                std::this_thread::sleep_for(std::chrono::microseconds(200));
            }
            return true;
        }

        // Waits (bounded) until a terminator was received
        bool waitForTerminator(std::chrono::milliseconds timeout = kTimeout)
        {
            auto deadline = std::chrono::steady_clock::now() + timeout;
            while (!st->got_terminator)
            {
                if (std::chrono::steady_clock::now() > deadline)
                    return false;
                std::this_thread::sleep_for(std::chrono::microseconds(200));
            }
            return true;
        }

        // Drains whatever is left, then stops the thread. Never blocks for long.
        void finish()
        {
            if (th.joinable())
            {
                st->stop = true;
                th.join();
            }
        }

        // Leaves the thread running (for hung tests)
        void abandon()
        {
            if (th.joinable())
            {
                leakForever(st);
                th.detach();
            }
        }

        std::vector<T> data()
        {
            std::lock_guard<std::mutex> lck(st->mtx);
            return st->data;
        }
        std::vector<uint32_t> sizes()
        {
            std::lock_guard<std::mutex> lck(st->mtx);
            return st->sizes;
        }
        std::vector<uint32_t> maxSizes()
        {
            std::lock_guard<std::mutex> lck(st->mtx);
            return st->max_sizes;
        }
        int terminators()
        {
            std::lock_guard<std::mutex> lck(st->mtx);
            return st->terminators;
        }
        int propagatingTerminators()
        {
            std::lock_guard<std::mutex> lck(st->mtx);
            return st->propagating_terminators;
        }
        int buffersAfterTerminator()
        {
            std::lock_guard<std::mutex> lck(st->mtx);
            return st->buffers_after_terminator;
        }
    };

    // Pushes a vector in chunks on its own thread, then an optional terminator
    template <typename T>
    class Feeder
    {
    private:
        struct State
        {
            std::shared_ptr<DSPStream> stream;
            std::vector<T> data;
            std::atomic<bool> done{false};
        };
        std::shared_ptr<State> st;
        std::thread th;

    public:
        // terminator : 0 = none, 1 = propagating, 2 = non-propagating
        Feeder(std::shared_ptr<DSPStream> s, std::vector<T> data, size_t chunk, int terminator = 1) : st(std::make_shared<State>())
        {
            st->stream = s;
            st->data = std::move(data);
            std::shared_ptr<State> lst = st;
            th = std::thread(
                [lst, chunk, terminator]()
                {
                    size_t pos = 0;
                    while (pos < lst->data.size())
                    {
                        size_t n = std::min(chunk, lst->data.size() - pos);
                        pushSamples<T>(*lst->stream, &lst->data[pos], n);
                        pos += n;
                    }
                    if (terminator != 0)
                        pushTerminator(*lst->stream, terminator == 1);
                    lst->done = true;
                });
        }

        ~Feeder()
        {
            if (th.joinable())
            {
                if (!wait())
                    abandon();
            }
        }

        Feeder(const Feeder &) = delete;
        Feeder &operator=(const Feeder &) = delete;

        bool wait(std::chrono::milliseconds timeout = kTimeout)
        {
            if (!th.joinable())
                return st->done;
            auto deadline = std::chrono::steady_clock::now() + timeout;
            while (!st->done)
            {
                if (std::chrono::steady_clock::now() > deadline)
                    return false;
                std::this_thread::sleep_for(std::chrono::microseconds(200));
            }
            th.join();
            return true;
        }

        void abandon()
        {
            if (th.joinable())
            {
                leakForever(st);
                th.detach();
            }
        }
    };

    // Calls blk->stop() (which joins the work thread), bounded by a timeout.
    // On timeout the block is leaked so its destructor never runs.
    inline bool stopBlock(std::shared_ptr<Block> blk, bool stop_now = false, std::chrono::milliseconds timeout = kTimeout)
    {
        auto done = std::make_shared<std::atomic<bool>>(false);
        std::thread th(
            [blk, done, stop_now]()
            {
                blk->stop(stop_now);
                *done = true;
            });
        auto deadline = std::chrono::steady_clock::now() + timeout;
        while (!*done)
        {
            if (std::chrono::steady_clock::now() > deadline)
            {
                leakForever(blk);
                th.detach();
                return false;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
        th.join();
        return true;
    }

    template <typename To>
    struct OutputResult
    {
        std::vector<To> data;
        std::vector<uint32_t> sizes;
        std::vector<uint32_t> max_sizes;
        int terminators = 0;
        int propagating_terminators = 0;
    };

    template <typename To>
    struct RunResult
    {
        bool completed = false;
        std::vector<OutputResult<To>> outputs;

        // Convenience for single output blocks
        std::vector<To> &out() { return outputs.at(0).data; }
    };

    /*
    Runs a whole block lifecycle :
    - every input i gets a new stream fed with inputs[i] (in chunks), followed by a terminator
    - every output gets a new stream with nbuf buffers, drained by a collector
    - start, feed, stop (join), collect
    terminator : 0 = none (then the block is stopped with stop_now), 1 = propagating, 2 = non-propagating
    */
    template <typename Ti, typename To>
    inline RunResult<To> runBlock(std::shared_ptr<Block> blk, const std::vector<std::vector<Ti>> &inputs, size_t chunk = 1024, int terminator = 1, int nbuf = 4)
    {
        RunResult<To> r;

        std::vector<std::shared_ptr<DSPStream>> in_streams;
        auto blk_inputs = blk->get_inputs();
        for (size_t i = 0; i < inputs.size(); i++)
        {
            BlockIO io = blk_inputs.at(i);
            io.fifo = std::make_shared<DSPStream>(nbuf);
            blk->set_input(io, (int)i);
            in_streams.push_back(io.fifo);
        }

        std::vector<std::unique_ptr<Collector<To>>> cols;
        size_t nout = blk->get_outputs().size();
        for (size_t o = 0; o < nout; o++)
            cols.push_back(std::make_unique<Collector<To>>(blk->get_output((int)o, nbuf).fifo));

        blk->start();

        std::vector<std::unique_ptr<Feeder<Ti>>> feeders;
        for (size_t i = 0; i < inputs.size(); i++)
            feeders.push_back(std::make_unique<Feeder<Ti>>(in_streams[i], inputs[i], chunk, terminator));

        bool ok = true;
        for (auto &f : feeders)
            ok = f->wait() && ok;

        if (ok)
            ok = stopBlock(blk, terminator == 0);
        else
            leakForever(blk);

        if (!ok)
        {
            for (auto &f : feeders)
                f->abandon();
            for (auto &c : cols)
                c->abandon();
            for (auto &s : in_streams)
                leakForever(s);
            r.completed = false;
            return r;
        }

        for (auto &c : cols)
        {
            c->finish();
            OutputResult<To> o;
            o.data = c->data();
            o.sizes = c->sizes();
            o.max_sizes = c->maxSizes();
            o.terminators = c->terminators();
            o.propagating_terminators = c->propagatingTerminators();
            r.outputs.push_back(o);
        }
        r.completed = true;
        return r;
    }

    // Single input / single output shortcut
    template <typename Ti, typename To>
    inline RunResult<To> runSimple(std::shared_ptr<Block> blk, const std::vector<Ti> &input, size_t chunk = 1024, int terminator = 1, int nbuf = 4)
    {
        return runBlock<Ti, To>(blk, {input}, chunk, terminator, nbuf);
    }

    // Helper to configure a block from a JSON object, returning the status
    inline Block::cfg_res_t configure(Block &blk, nlohmann::json cfg) { return blk.set_cfg(cfg); }
} // namespace ndsp_test
