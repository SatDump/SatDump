#pragma once

#include <complex>

namespace dsp
{
    class QuadratureDemod
    {
    private:
        float d_gain;

    public:
        QuadratureDemod(float gain);
        ~QuadratureDemod();

        void set_gain(float gain) { d_gain = gain; }
        float gain() const { return d_gain; }

        size_t work(std::complex<float> *in, size_t length, float *out);
    };
}; // namespace libdsp