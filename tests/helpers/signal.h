#pragma once

/*
Signal generation / analysis helpers for the DSP tests (common/dsp and the
new src-core/dsp framework alike). Everything is inline and lives in the
test_helpers namespace, as every test file is linked into the same executable.
Frequencies are in radians per sample.
*/

#include "common/dsp/complex.h"
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace test_helpers
{
    // Complex tone amp * e^(j(phase0 + freq * i))
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

    // Real cosine amp * cos(phase0 + freq * i)
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

    // Amplitude ratio to dB
    inline double toDb(double v) { return 20.0 * log10(v); }

    // Average frequency of a complex signal over [start, end)
    inline double estimateFreq(const std::vector<complex_t> &x, size_t start, size_t end)
    {
        std::complex<double> acc = 0;
        for (size_t i = start + 1; i < end; i++)
            acc += std::complex<double>(x[i].real, x[i].imag) * std::conj(std::complex<double>(x[i - 1].real, x[i - 1].imag));
        return std::arg(acc);
    }

    // Frequency of a real tone over [start, end), from zero crossings (0 if fewer than two)
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
        return M_PI * (crossings - 1) / (double)(last - first);
    }

    // Absolute difference of two samples (modulus of the difference for complex)
    inline float absDiff(float a, float b) { return std::fabs(a - b); }
    inline float absDiff(complex_t a, complex_t b) { return std::hypot(a.real - b.real, a.imag - b.imag); }
    inline float absDiff(int8_t a, int8_t b) { return std::fabs((float)a - (float)b); }
    inline float absDiff(int16_t a, int16_t b) { return std::fabs((float)a - (float)b); }
    inline float absDiff(uint8_t a, uint8_t b) { return std::fabs((float)a - (float)b); }
} // namespace test_helpers
