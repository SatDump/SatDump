#pragma once

/*
Shared helpers for the tests of the new DSP framework (src-core/dsp) blocks.
Everything is inline and lives in the ndsp_sig_test namespace, as every test
file is linked into the same executable.

The streaming harness is ndsp_test's (dsp/ndsp_test_helpers.h). runBlock /
runChain / RunOptions here only wrap it with the defaults these tests use.
*/

#include "common/dsp/complex.h"
#include "dsp/block.h"
#include "dsp/block_helpers.h"
#include "dsp/ndsp_test_helpers.h"
#include "helpers/random.h"
#include "helpers/signal.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>
#include <volk/volk_alloc.hh>

namespace ndsp_sig_test
{
    // Deterministic LCG, so failures are reproducible
    using test_helpers::Lcg;

    // Signal generation / analysis, frequencies in radians per sample
    using test_helpers::complexTone;
    using test_helpers::estimateFreq;
    using test_helpers::estimateRealFreq;
    using test_helpers::magResponse;
    using test_helpers::realTone;
    using test_helpers::toDb;

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

    // Applies a frequency offset (rad/sample) and phase to a complex signal
    inline std::vector<complex_t> rotate(const std::vector<complex_t> &x, double freq, double phase0 = 0.0)
    {
        std::vector<complex_t> v(x.size());
        for (size_t i = 0; i < x.size(); i++)
            v[i] = fromStd(toStd(x[i]) * std::polar(1.0, phase0 + freq * (double)i));
        return v;
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
        double s = std::sin(M_PI * t) / (M_PI * t);
        double d = 1.0 - 4.0 * beta * beta * t * t;
        if (std::abs(d) < 1e-9)
            return (M_PI / 4.0) * s;
        return s * std::cos(M_PI * beta * t) / d;
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
            v = fromStd(std::polar(1.0, M_PI / 8.0 + (rng.next() >> 29) * M_PI / 4.0));
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
            a -= M_PI / 4.0; // QPSK symbols at +-45 deg
        else if (order == 8)
            a -= M_PI / 8.0; // 8PSK symbols at 22.5 + k * 45 deg
        int k = (int)std::lround(a / (2.0 * M_PI / order));
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
                std::complex<double> r = std::polar(1.0, 2.0 * M_PI * rot / order);
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
    // Streaming harness (ndsp_test's, with the defaults of these tests)
    ///////////////////////////////////////////////////////////////////////////

    using ndsp_test::getAll;
    using ndsp_test::RunResult;
    using ndsp_test::setAll;

    // ndsp_test::RunOptions with other defaults : bigger chunks, input buffers always allocated for a
    // whole chunk, only output 0 collected, each collected output must deliver a terminator (the
    // results stop there) and a longer budget, as some of these runs are heavy
    struct RunOptions : ndsp_test::RunOptions
    {
        RunOptions()
        {
            chunk = 4096;
            full_chunk_buffers = true;
            all_outputs = false;
            out_indices = {0};
            until_output_terminator = true;
            timeout = ndsp_test::kLongTimeout;
        }
    };

    inline RunOptions chunked(size_t chunk)
    {
        RunOptions o;
        o.chunk = chunk;
        return o;
    }

    // Waits (with a timeout) until `count(idx)` reaches `n`. For RunOptions::before_terminate
    inline bool waitForCount(const std::function<size_t(size_t)> &count, size_t idx, size_t n, std::chrono::milliseconds timeout = ndsp_test::kLongTimeout)
    {
        return ndsp_test::waitUntil([&]() { return count(idx) >= n; }, timeout, std::chrono::milliseconds(1));
    }

    // Runs a chain of blocks : the input goes into chain.front(), each block is
    // linked to output 0 of the previous one, outputs are taken from chain.back()
    template <typename Ti, typename To>
    inline RunResult<To> runChain(const std::vector<std::shared_ptr<satdump::ndsp::Block>> &chain, const std::vector<Ti> &in, const RunOptions &opt = RunOptions())
    {
        return ndsp_test::runChain<Ti, To>(chain, {in}, opt);
    }

    template <typename Ti, typename To>
    inline RunResult<To> runBlock(std::shared_ptr<satdump::ndsp::Block> blk, const std::vector<Ti> &in, const RunOptions &opt = RunOptions())
    {
        return ndsp_test::runChain<Ti, To>({blk}, {in}, opt);
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
