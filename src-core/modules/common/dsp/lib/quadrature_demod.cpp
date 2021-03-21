#include "quadrature_demod.h"
#include <volk/volk.h>
#include <volk/volk_alloc.hh>
#include "fast_atan2f.h"

namespace dsp
{
    QuadratureDemod::QuadratureDemod(float gain) : d_gain(gain)
    {
    }

    QuadratureDemod::~QuadratureDemod()
    {
    }

    int QuadratureDemod::work(std::complex<float> *in, int length, float *out)
    {
        volk::vector<std::complex<float>> tmp(length);
        volk_32fc_x2_multiply_conjugate_32fc(&tmp[0], &in[1], &in[0], length);
        for (int i = 0; i < length; i++)
        {
            out[i] = d_gain * fast_atan2f(imag(tmp[i]), real(tmp[i]));
        }

        return length;
    }
}; // namespace libdsp