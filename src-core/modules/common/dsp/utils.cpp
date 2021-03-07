#include "utils.h"

namespace dsp
{
    float branchless_clip(float x, float clip)
    {
        return 0.5 * (std::abs(x + clip) - std::abs(x - clip));
    }

    void fast_cc_multiply(std::complex<float> &out, const std::complex<float> cc1, const std::complex<float> cc2)
    {
        // The built-in complex.h multiply has significant NaN/INF checking that
        // considerably slows down performance.  While on some compilers the
        // -fcx-limit-range flag can be used, this fast function makes the math consistent
        // in terms of performance for the Costas loop.
        float o_r, o_i;

        o_r = (cc1.real() * cc2.real()) - (cc1.imag() * cc2.imag());
        o_i = (cc1.real() * cc2.imag()) + (cc1.imag() * cc2.real());

        out.real(o_r);
        out.imag(o_i);
    }

    void sincosf(float x, float *sinx, float *cosx)
    {
        *sinx = ::sinf(x);
        *cosx = ::cosf(x);
    }

    std::complex<float> gr_expj(float phase)
    {
        float t_imag, t_real;
        sincosf(phase, &t_imag, &t_real);
        return std::complex<float>(t_real, t_imag);
    }
} // namespace libdsp