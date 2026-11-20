#pragma once

/*
Shared helpers for the tests of the new DSP framework (src-core/dsp) blocks.
Everything is inline and lives in the ndsp_sig_test namespace, as every test
file is linked into the same executable.

The streaming harness (runBlock) works like this :
- An input DSPStream is created and linked to the block's input 0, and the
  requested outputs are created with get_output(i, nbuf).
- The block is started, one collector thread per output drains it until it
  receives a terminator.
- The input is pushed in chunks, followed by a propagating terminator.
  The collectors are joined, then the block is stopped.
All of this runs in a worker thread, and the test thread only waits for it
with a (generous) timeout. If it times out, the worker is detached and
the block intentionally leaked (it may still be referenced by stuck threads),
so a hung block fails the test instead of hanging the whole suite.
*/

#include "common/dsp/complex.h"
#include "dsp/block.h"
#include "dsp/block_helpers.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <volk/volk_alloc.hh>

namespace ndsp_sig_test
{
    constexpr double kPi = 3.14159265358979323846;

    // Generous, as other heavy jobs may run on the machine at the same time
    constexpr std::chrono::milliseconds kTimeout(60000);

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
        double symmetric() { return 2.0 * uniform() - 1.0; }
        // Random +1 / -1
        float sign() { return (next() >> 31) ? 1.0f : -1.0f; }
        // Standard normal (Box-Muller)
        double gauss()
        {
            double u1 = std::max(uniform(), 1e-12), u2 = uniform();
            return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * kPi * u2);
        }
    };

    // Bit-exact comparison (complex_t has no operator==)
    inline bool sameSamples(const std::vector<complex_t> &a, const std::vector<complex_t> &b)
    {
        if (a.size() != b.size())
            return false;
        for (size_t i = 0; i < a.size(); i++)
            if (a[i].real != b[i].real || a[i].imag != b[i].imag)
                return false;
        return true;
    }

    inline std::complex<double> toStd(const complex_t &c) { return std::complex<double>(c.real, c.imag); }
    inline complex_t fromStd(const std::complex<double> &c) { return complex_t(c.real(), c.imag()); }

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

    // Applies a frequency offset (rad/sample) and phase to a complex signal
    inline std::vector<complex_t> rotate(const std::vector<complex_t> &x, double freq, double phase0 = 0.0)
    {
        std::vector<complex_t> v(x.size());
        for (size_t i = 0; i < x.size(); i++)
            v[i] = fromStd(toStd(x[i]) * std::polar(1.0, phase0 + freq * (double)i));
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
            acc += toStd(taps[n]) * std::polar(1.0, -w * (double)n);
        return std::abs(acc);
    }

    inline double toDb(double v) { return 20.0 * log10(v); }

    // Average frequency (rad/sample) of a complex signal over [start, end)
    inline double estimateFreq(const std::vector<complex_t> &x, size_t start, size_t end)
    {
        std::complex<double> acc = 0;
        for (size_t i = start + 1; i < end; i++)
            acc += toStd(x[i]) * std::conj(toStd(x[i - 1]));
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

    // Mean power over [start, end)
    inline double meanPower(const std::vector<complex_t> &x, size_t start, size_t end)
    {
        double acc = 0;
        for (size_t i = start; i < end; i++)
            acc += std::norm(toStd(x[i]));
        return acc / (double)(end - start);
    }

    inline double meanPower(const std::vector<float> &x, size_t start, size_t end)
    {
        double acc = 0;
        for (size_t i = start; i < end; i++)
            acc += (double)x[i] * x[i];
        return acc / (double)(end - start);
    }

    // Amplitude of the component at freq (rad/sample) of a complex signal over [start, end)
    inline double toneAmplitude(const std::vector<complex_t> &x, double freq, size_t start, size_t end)
    {
        std::complex<double> acc = 0;
        for (size_t i = start; i < end; i++)
            acc += toStd(x[i]) * std::polar(1.0, -freq * (double)i);
        return std::abs(acc) / (double)(end - start);
    }

    // Amplitude of a real cosine at freq (rad/sample) over [start, end)
    inline double toneAmplitude(const std::vector<float> &x, double freq, size_t start, size_t end)
    {
        std::complex<double> acc = 0;
        for (size_t i = start; i < end; i++)
            acc += (double)x[i] * std::polar(1.0, -freq * (double)i);
        return 2.0 * std::abs(acc) / (double)(end - start);
    }

    // Reference "valid part" convolution, matching how the ndsp FIRBlock outputs :
    // out[i] = sum_j taps[j] * x[i + ntaps - j], for i in [0, x.size() - ntaps)
    template <typename T, typename TT>
    inline std::vector<std::complex<double>> firReference(const std::vector<T> &x, const std::vector<TT> &taps)
    {
        auto toc = [](const auto &v) -> std::complex<double>
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, complex_t>)
                return toStd(v);
            else
                return std::complex<double>(v, 0);
        };
        size_t nt = taps.size();
        std::vector<std::complex<double>> out;
        for (size_t i = 0; i + nt < x.size(); i++)
        {
            std::complex<double> acc = 0;
            for (size_t j = 0; j < nt; j++)
                acc += toc(taps[j]) * toc(x[i + nt - j]);
            out.push_back(acc);
        }
        return out;
    }

    // Full linear convolution of real sequences
    inline std::vector<double> convolveFull(const std::vector<double> &a, const std::vector<double> &b)
    {
        std::vector<double> out(a.size() + b.size() - 1, 0.0);
        for (size_t i = 0; i < a.size(); i++)
            for (size_t j = 0; j < b.size(); j++)
                out[i + j] += a[i] * b[j];
        return out;
    }

    // Raised-cosine pulse, t in symbol periods
    inline double raisedCosine(double t, double beta)
    {
        if (std::abs(t) < 1e-9)
            return 1.0;
        double s = std::sin(kPi * t) / (kPi * t);
        double d = 1.0 - 4.0 * beta * beta * t * t;
        if (std::abs(d) < 1e-9)
            return (kPi / 4.0) * s;
        return s * std::cos(kPi * beta * t) / d;
    }

    // Symbols shaped with raised-cosine pulses at `sps` samples per symbol (non integer allowed),
    // symbol k peaking at sample index (k + offset) * sps. Pulses truncated to +-span symbols.
    inline std::vector<complex_t> rcShape(const std::vector<complex_t> &syms, double sps, double offset = 0.0, double beta = 0.5, int span = 8)
    {
        size_t n = (size_t)std::floor(syms.size() * sps);
        std::vector<std::complex<double>> acc(n, 0.0);
        for (size_t k = 0; k < syms.size(); k++)
        {
            double center = ((double)k + offset) * sps;
            long lo = std::max<long>(0, (long)std::floor(center - span * sps));
            long hi = std::min<long>((long)n - 1, (long)std::ceil(center + span * sps));
            for (long i = lo; i <= hi; i++)
                acc[i] += toStd(syms[k]) * raisedCosine(((double)i - center) / sps, beta);
        }
        std::vector<complex_t> out(n);
        for (size_t i = 0; i < n; i++)
            out[i] = fromStd(acc[i]);
        return out;
    }

    inline std::vector<float> rcShapeReal(const std::vector<float> &syms, double sps, double offset = 0.0, double beta = 0.5, int span = 8)
    {
        std::vector<complex_t> c(syms.size());
        for (size_t i = 0; i < syms.size(); i++)
            c[i] = complex_t(syms[i], 0);
        auto s = rcShape(c, sps, offset, beta, span);
        std::vector<float> out(s.size());
        for (size_t i = 0; i < s.size(); i++)
            out[i] = s[i].real;
        return out;
    }

    // Repeat each symbol `sps` times (rectangular pulses), integer sps only
    inline std::vector<complex_t> rectShape(const std::vector<complex_t> &syms, int sps)
    {
        std::vector<complex_t> out;
        for (auto &s : syms)
            for (int i = 0; i < sps; i++)
                out.push_back(s);
        return out;
    }

    // Filter with real taps, output aligned with input (zero history, same length)
    inline std::vector<complex_t> filterSame(const std::vector<complex_t> &x, const std::vector<float> &taps)
    {
        std::vector<complex_t> out(x.size());
        for (size_t i = 0; i < x.size(); i++)
        {
            std::complex<double> acc = 0;
            for (size_t j = 0; j < taps.size() && j <= i; j++)
                acc += (double)taps[j] * toStd(x[i - j]);
            out[i] = fromStd(acc);
        }
        return out;
    }

    inline std::vector<complex_t> bpskSymbols(size_t n, uint32_t seed)
    {
        Lcg rng(seed);
        std::vector<complex_t> s(n);
        for (auto &v : s)
            v = complex_t(rng.sign(), 0);
        return s;
    }

    inline std::vector<complex_t> qpskSymbols(size_t n, uint32_t seed)
    {
        Lcg rng(seed);
        const float a = 1.0f / std::sqrt(2.0f);
        std::vector<complex_t> s(n);
        for (auto &v : s)
        {
            float re = rng.sign() * a;
            float im = rng.sign() * a;
            v = complex_t(re, im);
        }
        return s;
    }

    // 8PSK at 22.5 + k * 45 degrees, where the usual 8PSK Costas detector locks
    inline std::vector<complex_t> psk8Symbols(size_t n, uint32_t seed)
    {
        Lcg rng(seed);
        std::vector<complex_t> s(n);
        for (auto &v : s)
            v = fromStd(std::polar(1.0, kPi / 8.0 + (rng.next() >> 29) * kPi / 4.0));
        return s;
    }

    inline void addNoise(std::vector<complex_t> &x, double sigma_per_component, uint32_t seed)
    {
        Lcg rng(seed);
        for (auto &v : x)
        {
            v.real += sigma_per_component * rng.gauss();
            v.imag += sigma_per_component * rng.gauss();
        }
    }

    // Best agreement between hard decisions of `ref` symbols and `out` over the last
    // `window` output symbols, searching lags in [0, max_lag] and all `nrot` phase rotations.
    // QPSK/BPSK decisions are made in the respective quadrant/half plane.
    struct MatchResult
    {
        double ratio = 0;
        int lag = 0;
        int rot = 0;
    };

    inline int decide(std::complex<double> v, int order)
    {
        if (order == 2)
            return v.real() >= 0 ? 0 : 1;
        // M-PSK sector decision
        double a = std::arg(v);
        if (order == 4)
            a -= kPi / 4.0; // QPSK symbols at +-45 deg
        else if (order == 8)
            a -= kPi / 8.0; // 8PSK symbols at 22.5 + k * 45 deg
        int k = (int)std::lround(a / (2.0 * kPi / order));
        return ((k % order) + order) % order;
    }

    inline MatchResult matchSymbols(const std::vector<complex_t> &ref, const std::vector<complex_t> &out, int order, size_t window, int max_lag)
    {
        MatchResult best;
        if (out.size() < window)
            return best;
        size_t ostart = out.size() - window;
        for (int lag = -max_lag; lag <= max_lag; lag++)
        {
            for (int rot = 0; rot < order; rot++)
            {
                std::complex<double> r = std::polar(1.0, 2.0 * kPi * rot / order);
                size_t good = 0, total = 0;
                for (size_t i = ostart; i < out.size(); i++)
                {
                    long ri = (long)i + lag;
                    if (ri < 0 || ri >= (long)ref.size())
                        continue;
                    total++;
                    if (decide(toStd(out[i]) * r, order) == decide(toStd(ref[ri]), order))
                        good++;
                }
                if (total < window / 2)
                    continue;
                double ratio = (double)good / (double)total;
                if (ratio > best.ratio)
                    best = {ratio, lag, rot};
            }
        }
        return best;
    }

    inline std::vector<complex_t> toComplex(const std::vector<float> &x)
    {
        std::vector<complex_t> v(x.size());
        for (size_t i = 0; i < x.size(); i++)
            v[i] = complex_t(x[i], 0);
        return v;
    }

    // Index of the largest value
    template <typename T>
    inline size_t argmax(const std::vector<T> &v)
    {
        return std::max_element(v.begin(), v.end()) - v.begin();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Streaming harness
    ///////////////////////////////////////////////////////////////////////////

    template <typename To>
    struct RunResult
    {
        bool completed = false;
        std::vector<std::vector<To>> outputs;              // One per requested output
        std::vector<std::vector<uint32_t>> buffer_sizes;   // Sizes of each output buffer received
        std::vector<To> &output() { return outputs[0]; }   // Shortcut for single-output blocks
    };

    struct RunOptions
    {
        size_t chunk = 4096;              // Samples per input buffer
        std::vector<size_t> chunk_cycle;  // If not empty, cycles through these chunk sizes instead
        std::vector<int> out_indices = {0}; // Outputs to collect. Empty for sinks
        int nbuf = 4;                     // Buffers per stream
        bool create_outputs = true;       // Call get_output(i, nbuf), otherwise get_output(i, 0)
        // Called after all the input is queued, before the terminator is sent.
        // The argument returns the number of samples collected so far on output #i
        std::function<void(const std::function<size_t(size_t)> &)> before_terminate;
    };

    // The blocks' set_cfg(key, v) / get_cfg(key) hide the Block-level overloads
    // taking / returning every parameter at once, so call those through the base class
    inline satdump::ndsp::Block::cfg_res_t setAll(satdump::ndsp::Block &b, const nlohmann::json &v) { return b.set_cfg(v); }
    inline nlohmann::json getAll(satdump::ndsp::Block &b) { return b.get_cfg(); }

    inline RunOptions chunked(size_t chunk)
    {
        RunOptions o;
        o.chunk = chunk;
        return o;
    }

    // Waits (with a timeout) until `count(idx)` reaches `n`
    inline bool waitForCount(const std::function<size_t(size_t)> &count, size_t idx, size_t n, std::chrono::milliseconds timeout = kTimeout)
    {
        auto start = std::chrono::steady_clock::now();
        while (count(idx) < n)
        {
            if (std::chrono::steady_clock::now() - start > timeout)
                return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return true;
    }

    template <typename To>
    struct SharedRunState
    {
        std::mutex mtx;
        std::vector<std::vector<To>> outputs;
        std::vector<std::vector<uint32_t>> sizes;
        std::vector<std::unique_ptr<std::atomic<size_t>>> counts;
    };

    // Runs a chain of blocks : the input goes into chain.front(), each block is
    // linked to output 0 of the previous one, outputs are taken from chain.back()
    template <typename Ti, typename To>
    inline RunResult<To> runChain(std::vector<std::shared_ptr<satdump::ndsp::Block>> chain, const std::vector<Ti> &in, RunOptions opt = RunOptions())
    {
        using namespace satdump::ndsp;

        auto st = std::make_shared<SharedRunState<To>>();
        size_t nout = opt.out_indices.size();
        st->outputs.resize(nout);
        st->sizes.resize(nout);
        for (size_t i = 0; i < nout; i++)
            st->counts.emplace_back(std::make_unique<std::atomic<size_t>>(0));

        auto prom = std::make_shared<std::promise<void>>();
        auto fut = prom->get_future();
        auto input = std::make_shared<std::vector<Ti>>(in);

        std::thread worker(
            [chain, input, opt, st, prom, nout]()
            {
                bool started = false;
                std::vector<std::thread> collectors;
                std::shared_ptr<DSPStream> ififo;
                try
                {
                    BlockIO istr;
                    istr.name = "test_in";
                    istr.type = getTypeSampleType<Ti>();
                    istr.fifo = std::make_shared<DSPStream>(opt.nbuf);
                    ififo = istr.fifo;
                    chain.front()->set_input(istr, 0);
                    for (size_t i = 1; i < chain.size(); i++)
                        chain[i]->link(chain[i - 1].get(), 0, 0, opt.nbuf);

                    std::vector<std::shared_ptr<DSPStream>> ofifos;
                    for (size_t o = 0; o < nout; o++)
                        ofifos.push_back(chain.back()->get_output(opt.out_indices[o], opt.create_outputs ? opt.nbuf : 0).fifo);

                    for (auto &b : chain)
                        b->start();
                    started = true;

                    for (size_t o = 0; o < nout; o++)
                    {
                        collectors.emplace_back(
                            [st, o, f = ofifos[o]]()
                            {
                                while (true)
                                {
                                    DSPBuffer b = f->wait_dequeue();
                                    if (b.isTerminator())
                                    {
                                        f->free(b);
                                        break;
                                    }
                                    {
                                        std::lock_guard<std::mutex> lg(st->mtx);
                                        To *p = b.getSamples<To>();
                                        st->outputs[o].insert(st->outputs[o].end(), p, p + b.size);
                                        st->sizes[o].push_back(b.size);
                                    }
                                    *st->counts[o] += b.size;
                                    f->free(b);
                                }
                            });
                    }

                    // Feed the input
                    size_t pos = 0, cyc = 0;
                    while (pos < input->size())
                    {
                        size_t csz = opt.chunk_cycle.empty() ? opt.chunk : opt.chunk_cycle[cyc++ % opt.chunk_cycle.size()];
                        size_t n = std::min(csz, input->size() - pos);
                        DSPBuffer b = ififo->newBufferSamples(csz, sizeof(Ti));
                        memcpy(b.getSamples<Ti>(), input->data() + pos, n * sizeof(Ti));
                        b.size = n;
                        ififo->wait_enqueue(b);
                        pos += n;
                    }

                    if (opt.before_terminate)
                        opt.before_terminate([st](size_t i) { return (size_t)*st->counts[i]; });

                    ififo->wait_enqueue(ififo->newBufferTerminator());
                    for (auto &c : collectors)
                        c.join();
                    for (auto &b : chain)
                        b->stop();
                    prom->set_value();
                }
                catch (...)
                {
                    // Best effort cleanup, so that the block isn't destroyed while running
                    if (started)
                    {
                        ififo->wait_enqueue(ififo->newBufferTerminator());
                        for (auto &c : collectors)
                            if (c.joinable())
                                c.join();
                        for (auto &b : chain)
                            b->stop();
                    }
                    prom->set_exception(std::current_exception());
                }
            });

        RunResult<To> r;
        if (fut.wait_for(kTimeout) == std::future_status::ready)
        {
            worker.join();
            fut.get(); // Rethrows anything thrown in the worker
            r.completed = true;
            std::lock_guard<std::mutex> lg(st->mtx);
            r.outputs = st->outputs;
            r.buffer_sizes = st->sizes;
        }
        else
        {
            // Hung. Leak the worker and block, so we can keep going
            worker.detach();
            r.outputs.resize(nout);
            r.buffer_sizes.resize(nout);
        }
        return r;
    }

    template <typename Ti, typename To>
    inline RunResult<To> runBlock(std::shared_ptr<satdump::ndsp::Block> blk, const std::vector<Ti> &in, RunOptions opt = RunOptions())
    {
        return runChain<Ti, To>({blk}, in, opt);
    }

    // Calls process() directly on a BlockSimple-like block, in chunks,
    // with volk-aligned buffers. init() must have been called by the caller.
    template <typename To, typename Ti, typename B>
    inline std::vector<To> processDirect(B &blk, const std::vector<Ti> &in, size_t chunk, double out_ratio = 1.0)
    {
        std::vector<To> res;
        volk::vector<Ti> ibuf(chunk);
        volk::vector<To> obuf((size_t)std::ceil(chunk * out_ratio) + 16);
        for (size_t pos = 0; pos < in.size(); pos += chunk)
        {
            size_t n = std::min(chunk, in.size() - pos);
            std::copy(in.begin() + pos, in.begin() + pos + n, ibuf.begin());
            uint32_t no = blk.process(ibuf.data(), n, obuf.data());
            res.insert(res.end(), obuf.begin(), obuf.begin() + no);
        }
        return res;
    }
} // namespace ndsp_sig_test
