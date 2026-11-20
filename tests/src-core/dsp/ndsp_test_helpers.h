#pragma once

/*
Shared helpers for the tests of the new DSP framework (src-core/dsp).
Everything lives in the ndsp_test namespace and is inline, as every test file
is linked into the same executable.

This header holds the only streaming harness of these tests (ndsp_sig_test
wraps it with other defaults). It works like this :
- A Collector thread drains one block output (a DSPStream) into a vector,
  recording buffer sizes and terminators. It polls with try_dequeue so it
  can always be stopped.
- A Feeder thread pushes the input vector into a DSPStream in chunks, then
  pushes a terminator.
- runChain() / runBlock() / runSimple() wire the block(s) up, start
  everything, wait for the feeders, then stop (join) the blocks and finally
  drain and stop the collectors.
Every blocking wait is bounded by a timeout. If something hangs, the test
fails and the stuck block / threads are intentionally leaked (never
destroyed, threads detached) so that the rest of the suite can still run.
*/

#include "common/dsp/complex.h"
#include "dsp/base/dsp_buffer.h"
#include "dsp/base/stream.h"
#include "dsp/block.h"
#include "dsp/block_helpers.h"
#include "helpers/random.h"
#include "helpers/signal.h"
#include "helpers/temp_files.h"
#include "helpers/wait.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
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

    // Generous (20 s), as other heavy jobs may run on the machine at the same time
    using test_helpers::kTimeout;

    // Deterministic LCG, so failures are reproducible
    using test_helpers::Lcg;

    ///////////////////////////////////////////////////////////////////////////
    // Signal generation / analysis
    ///////////////////////////////////////////////////////////////////////////

    inline std::vector<float> randomFloats(size_t n, uint32_t seed = 1, float scale = 1.0f)
    {
        Lcg rng(seed);
        std::vector<float> v(n);
        for (auto &x : v)
            x = (float)rng.symmetric() * scale;
        return v;
    }

    inline std::vector<complex_t> randomComplex(size_t n, uint32_t seed = 1, float scale = 1.0f)
    {
        Lcg rng(seed);
        std::vector<complex_t> v(n);
        for (auto &x : v)
        {
            float re = (float)rng.symmetric() * scale;
            float im = (float)rng.symmetric() * scale;
            x = complex_t(re, im);
        }
        return v;
    }

    // Tones and frequency estimation, freq in radians per sample
    using test_helpers::complexTone;
    using test_helpers::estimateFreq;
    using test_helpers::realTone;

    // Before maxAbsDiff, which calls it unqualified
    using test_helpers::absDiff;

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

    // Unique temporary path (the file is not created), removed (if it exists) on destruction. .str() is the path
    using test_helpers::TempFile;

    // Raw element bytes of a vector to / from a file
    using test_helpers::readBytes;
    using test_helpers::writeBytes;

    ///////////////////////////////////////////////////////////////////////////
    // Configuration
    ///////////////////////////////////////////////////////////////////////////

    // The blocks' set_cfg(key, v) / get_cfg(key) hide the Block-level overloads
    // taking / returning every parameter at once, so call those through the base class
    inline Block::cfg_res_t setAll(Block &b, const nlohmann::json &v) { return b.set_cfg(v); }
    inline nlohmann::json getAll(Block &b) { return b.get_cfg(); }

    // Same as setAll (older name, kept for the tests using it)
    inline Block::cfg_res_t configure(Block &blk, const nlohmann::json &cfg) { return setAll(blk, cfg); }

    ///////////////////////////////////////////////////////////////////////////
    // Streaming harness
    ///////////////////////////////////////////////////////////////////////////

    // Budget of a whole run of heavy signal processing blocks (chains, hier blocks...),
    // longer than kTimeout on purpose. The default of the ndsp_sig_test wrappers
    constexpr std::chrono::milliseconds kLongTimeout(60000);

    // Keeps something alive forever. Used for blocks / streams of a hung test, as
    // destroying a block whose thread is still running throws (and terminates)
    template <typename T>
    inline void leakForever(std::shared_ptr<T> p)
    {
        new std::shared_ptr<T>(p);
    }

    // Polls a predicate until it is true, bounded by a timeout
    using test_helpers::waitUntil;

    // Time left before a deadline (0 once it passed), for waits sharing one budget
    inline std::chrono::milliseconds timeLeft(std::chrono::steady_clock::time_point deadline)
    {
        auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
        return std::max(left, std::chrono::milliseconds(0));
    }

    // Copies samples into a DSPStream buffer and enqueues it. May block on allocation.
    // The buffer is allocated for max(n, min_alloc) samples (its max_size)
    template <typename T>
    inline void pushSamples(DSPStream &s, const T *data, size_t n, size_t min_alloc = 0)
    {
        DSPBuffer b = s.newBufferSamples(std::max(n, min_alloc), sizeof(T));
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
            // Buffers / samples received before the first terminator
            size_t buffers_before_terminator = 0;
            size_t samples_before_terminator = 0;
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
                                    if (lst->terminators == 0)
                                    {
                                        lst->buffers_before_terminator = lst->sizes.size();
                                        lst->samples_before_terminator = lst->data.size();
                                    }
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
            return waitUntil([this, n]() { return size() >= n; }, timeout);
        }

        // Waits (bounded) until a terminator was received
        bool waitForTerminator(std::chrono::milliseconds timeout = kTimeout)
        {
            return waitUntil([this]() { return gotTerminator(); }, timeout);
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
        size_t buffersBeforeTerminator()
        {
            std::lock_guard<std::mutex> lck(st->mtx);
            return st->buffers_before_terminator;
        }
        size_t samplesBeforeTerminator()
        {
            std::lock_guard<std::mutex> lck(st->mtx);
            return st->samples_before_terminator;
        }
    };

    // How a Feeder splits its input into buffers
    struct FeedOptions
    {
        size_t chunk = 1024;             // Samples per buffer
        std::vector<size_t> chunk_cycle; // If not empty, cycles through these chunk sizes instead
        bool full_chunk_buffers = false; // Allocate every buffer for a whole chunk (max_size), even the shorter last one
        int terminator = 1;              // Sent after the data. 0 = none, 1 = propagating, 2 = non-propagating
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
            std::atomic<size_t> fed{0}; // Samples queued so far
            std::atomic<bool> done{false};
        };
        std::shared_ptr<State> st;
        std::thread th;

    public:
        // terminator : 0 = none, 1 = propagating, 2 = non-propagating
        Feeder(std::shared_ptr<DSPStream> s, std::vector<T> data, size_t chunk, int terminator = 1) : Feeder(s, std::move(data), FeedOptions{chunk, {}, false, terminator}) {}

        Feeder(std::shared_ptr<DSPStream> s, std::vector<T> data, FeedOptions opt) : st(std::make_shared<State>())
        {
            st->stream = s;
            st->data = std::move(data);
            std::shared_ptr<State> lst = st;
            th = std::thread(
                [lst, opt]()
                {
                    size_t pos = 0, cyc = 0;
                    while (pos < lst->data.size())
                    {
                        size_t csz = opt.chunk_cycle.empty() ? opt.chunk : opt.chunk_cycle[cyc++ % opt.chunk_cycle.size()];
                        size_t n = std::min(csz, lst->data.size() - pos);
                        pushSamples<T>(*lst->stream, &lst->data[pos], n, opt.full_chunk_buffers ? csz : n);
                        pos += n;
                        lst->fed = pos;
                    }
                    if (opt.terminator != 0)
                        pushTerminator(*lst->stream, opt.terminator == 1);
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

        size_t fed() { return st->fed; }
        size_t total() { return st->data.size(); }

        bool wait(std::chrono::milliseconds timeout = kTimeout)
        {
            if (!th.joinable())
                return st->done;
            if (!waitUntil([this]() { return (bool)st->done; }, timeout))
                return false;
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
        if (!waitUntil([done]() { return (bool)*done; }, timeout))
        {
            leakForever(blk);
            th.detach();
            return false;
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
        std::vector<OutputResult<To>> outputs; // One per collected output (empty ones if the run did not complete)

        // Convenience for single output blocks (both names are used)
        std::vector<To> &out() { return outputs.at(0).data; }
        std::vector<To> &output() { return out(); }
    };

    struct RunOptions
    {
        // Input feeding, see FeedOptions
        size_t chunk = 1024;
        std::vector<size_t> chunk_cycle;
        bool full_chunk_buffers = false;
        int terminator = 1; // 0 = none (then the blocks are stopped with stop_now), 1 = propagating, 2 = non-propagating
        // Called after all the input is queued, before the terminators are sent.
        // The argument returns the number of samples collected so far on collected output #i
        std::function<void(const std::function<size_t(size_t)> &)> before_terminate;

        int nbuf = 4; // Buffers of every stream

        // Outputs of the last block to collect : all of them, or only out_indices (empty for sinks)
        bool all_outputs = true;
        std::vector<int> out_indices;

        // Every collected output must deliver a terminator before the blocks are stopped (the run
        // fails otherwise), and the results stop at that first terminator
        bool until_output_terminator = false;

        // Budget of the whole run
        std::chrono::milliseconds timeout = kTimeout;
    };

    /*
    Runs the lifecycle of a chain of blocks : inputs[i] goes into input i of chain.front(),
    each next block is linked to output 0 of the previous one, outputs are taken from chain.back().
    - every input gets a new stream with nbuf buffers (typed after Ti, which set_input() checks)
    - every collected output gets a new stream with nbuf buffers, drained by a Collector
    - start every block, feed (Feeder threads), optionally wait for the output terminators,
      stop (join) every block in order, then drain and stop the collectors
    Every wait shares the opt.timeout budget. If it runs out, a diagnostic is printed, completed is
    false and every block, stream and thread is leaked, so the rest of the suite can still run.
    */
    template <typename Ti, typename To>
    inline RunResult<To> runChain(const std::vector<std::shared_ptr<Block>> &chain, const std::vector<std::vector<Ti>> &inputs, const RunOptions &opt)
    {
        RunResult<To> r;
        auto deadline = std::chrono::steady_clock::now() + opt.timeout;
        std::shared_ptr<Block> front = chain.at(0), back = chain.back();

        std::vector<std::shared_ptr<DSPStream>> in_streams;
        auto blk_inputs = front->get_inputs();
        for (size_t i = 0; i < inputs.size(); i++)
        {
            BlockIO io = blk_inputs.at(i);
            io.type = satdump::ndsp::getTypeSampleType<Ti>();
            io.fifo = std::make_shared<DSPStream>(opt.nbuf);
            front->set_input(io, (int)i);
            in_streams.push_back(io.fifo);
        }
        for (size_t i = 1; i < chain.size(); i++)
            chain[i]->link(chain[i - 1].get(), 0, 0, opt.nbuf);

        std::vector<int> outs = opt.out_indices;
        if (opt.all_outputs)
        {
            outs.clear();
            for (size_t o = 0; o < back->get_outputs().size(); o++)
                outs.push_back((int)o);
        }
        std::vector<std::unique_ptr<Collector<To>>> cols;
        for (int o : outs)
            cols.push_back(std::make_unique<Collector<To>>(back->get_output(o, opt.nbuf).fifo));

        size_t started = 0;
        try
        {
            for (auto &b : chain)
            {
                b->start();
                started++;
            }
        }
        catch (...)
        {
            // Never destroy a running block
            if (started > 0)
            {
                for (auto &b : chain)
                    leakForever(b);
                for (auto &s : in_streams)
                    leakForever(s);
                for (auto &c : cols)
                    c->abandon();
            }
            throw;
        }

        std::vector<std::unique_ptr<Feeder<Ti>>> feeders;
        std::exception_ptr hook_error;

        // Hung (or very slow) : leak everything and tell a stalled run from a slow one
        auto fail = [&](const std::string &stage)
        {
            size_t fed = 0, total = 0;
            for (size_t i = 0; i < feeders.size() && i < inputs.size(); i++)
                fed += feeders[i]->fed();
            for (auto &in : inputs)
                total += in.size();
            std::string collected;
            for (auto &c : cols)
                collected += " " + std::to_string(c->size());
            fprintf(stderr, "ndsp_test::runChain timed out while %s (first block %s): %zu of %zu input samples queued, output samples collected:%s\n", stage.c_str(), front->d_id.c_str(), fed, total,
                    collected.empty() ? " (no outputs)" : collected.c_str());
            for (auto &b : chain)
                leakForever(b);
            for (auto &s : in_streams)
                leakForever(s);
            for (auto &f : feeders)
                f->abandon();
            for (auto &c : cols)
                c->abandon();
            r.completed = false;
            r.outputs.resize(cols.size());
            if (hook_error)
                std::rethrow_exception(hook_error);
            return r;
        };

        // With a before_terminate hook, the terminators are sent by a second set of feeders
        FeedOptions fo{opt.chunk, opt.chunk_cycle, opt.full_chunk_buffers, opt.before_terminate ? 0 : opt.terminator};
        for (size_t i = 0; i < inputs.size(); i++)
            feeders.push_back(std::make_unique<Feeder<Ti>>(in_streams[i], inputs[i], fo));
        for (auto &f : feeders)
            if (!f->wait(timeLeft(deadline)))
                return fail("feeding the input");

        if (opt.before_terminate)
        {
            try
            {
                opt.before_terminate([&cols](size_t i) { return cols.at(i)->size(); });
            }
            catch (...)
            {
                // Rethrown once the blocks are stopped
                hook_error = std::current_exception();
            }
            if (opt.terminator != 0)
            {
                FeedOptions to;
                to.terminator = opt.terminator;
                size_t first = feeders.size();
                for (auto &s : in_streams)
                    feeders.push_back(std::make_unique<Feeder<Ti>>(s, std::vector<Ti>(), to));
                for (size_t i = first; i < feeders.size(); i++)
                    if (!feeders[i]->wait(timeLeft(deadline)))
                        return fail("sending the terminators");
            }
        }

        if (opt.until_output_terminator)
            for (auto &c : cols)
                if (!c->waitForTerminator(timeLeft(deadline)))
                    return fail("waiting for the output terminators");

        for (auto &b : chain)
            if (!stopBlock(b, opt.terminator == 0, timeLeft(deadline)))
                return fail("stopping block " + b->d_id);

        for (auto &c : cols)
        {
            c->finish();
            OutputResult<To> o;
            o.data = c->data();
            o.sizes = c->sizes();
            o.max_sizes = c->maxSizes();
            o.terminators = c->terminators();
            o.propagating_terminators = c->propagatingTerminators();
            if (opt.until_output_terminator)
            {
                o.data.resize(c->samplesBeforeTerminator());
                o.sizes.resize(c->buffersBeforeTerminator());
                o.max_sizes.resize(c->buffersBeforeTerminator());
            }
            r.outputs.push_back(std::move(o));
        }
        r.completed = true;
        if (hook_error)
            std::rethrow_exception(hook_error);
        return r;
    }

    template <typename Ti, typename To>
    inline RunResult<To> runBlock(std::shared_ptr<Block> blk, const std::vector<std::vector<Ti>> &inputs, const RunOptions &opt)
    {
        return runChain<Ti, To>({blk}, inputs, opt);
    }

    /*
    Runs a whole block lifecycle (see runChain) :
    - every input i gets a new stream fed with inputs[i] (in chunks), followed by a terminator
    - every output gets a new stream with nbuf buffers, drained by a collector
    - start, feed, stop (join), collect
    terminator : 0 = none (then the block is stopped with stop_now), 1 = propagating, 2 = non-propagating
    */
    template <typename Ti, typename To>
    inline RunResult<To> runBlock(std::shared_ptr<Block> blk, const std::vector<std::vector<Ti>> &inputs, size_t chunk = 1024, int terminator = 1, int nbuf = 4)
    {
        RunOptions opt;
        opt.chunk = chunk;
        opt.terminator = terminator;
        opt.nbuf = nbuf;
        return runBlock<Ti, To>(blk, inputs, opt);
    }

    // Single input / single output shortcut
    template <typename Ti, typename To>
    inline RunResult<To> runSimple(std::shared_ptr<Block> blk, const std::vector<Ti> &input, size_t chunk = 1024, int terminator = 1, int nbuf = 4)
    {
        return runBlock<Ti, To>(blk, {input}, chunk, terminator, nbuf);
    }
} // namespace ndsp_test
