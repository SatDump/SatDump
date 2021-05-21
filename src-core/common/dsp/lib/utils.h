#pragma once

#include <complex>

namespace dsp
{
    float branchless_clip(float x, float clip);
    void fast_cc_multiply(std::complex<float> &out, const std::complex<float> cc1, const std::complex<float> cc2);
    void sincosf(float x, float *sinx, float *cosx);
    std::complex<float> gr_expj(float phase);
} // namespace libdsp