#pragma once

/*
Shared signal generator for the clock recovery tests.
Raised-cosine shaped symbols evaluated at arbitrary (fractional) samples per symbol,
so the recovered samples are ISI free when sampled at the right instant.
*/

#include "../dsp_test_helpers.h"
#include <cmath>
#include <vector>

namespace dsp_test_clock
{
    inline double raisedCosine(double t, double beta)
    {
        if (fabs(t) < 1e-9)
            return 1.0;
        double d = 1.0 - (2 * beta * t) * (2 * beta * t);
        double s = sin(dsp_test::kPi * t) / (dsp_test::kPi * t);
        if (fabs(d) < 1e-9)
            return (dsp_test::kPi / 4.0) * s; // Limit at t = +-1/(2 beta)
        return s * cos(dsp_test::kPi * beta * t) / d;
    }

    inline std::vector<float> randomSymbols(size_t n, uint32_t seed)
    {
        dsp_test::Lcg rng(seed);
        std::vector<float> s(n);
        for (auto &v : s)
            v = rng.sign();
        return s;
    }

    // Samples sum_k sym[k] p((n - offset) / sps - k)
    inline std::vector<float> shapeSymbols(const std::vector<float> &sym, double sps, double offset, double beta = 0.5)
    {
        size_t n = (size_t)(sym.size() * sps);
        std::vector<float> out(n);
        for (size_t i = 0; i < n; i++)
        {
            double t = ((double)i - offset) / sps;
            long k0 = (long)floor(t);
            double acc = 0;
            for (long k = k0 - 10; k <= k0 + 10; k++)
                if (k >= 0 && k < (long)sym.size())
                    acc += sym[k] * raisedCosine(t - k, beta);
            out[i] = acc;
        }
        return out;
    }

    // Best match ratio between hard decisions of rec[start..] and sym[start + lag], over a lag range
    template <typename F>
    inline double bestMatch(size_t nrec, size_t start, const std::vector<float> &sym, F decision, int max_lag = 20)
    {
        double best = 0;
        for (int lag = -max_lag; lag <= max_lag; lag++)
        {
            size_t ok = 0, total = 0;
            for (size_t k = start; k < nrec; k++)
            {
                long j = (long)k + lag;
                if (j < 0 || j >= (long)sym.size())
                    continue;
                total++;
                ok += (decision(k) == (sym[j] > 0));
            }
            if (total > 0)
                best = std::max(best, (double)ok / total);
        }
        return best;
    }
}
