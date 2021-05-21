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

        int work(std::complex<float> *in, int length, float *out);
    };
}; // namespace libdsp