#pragma once
#include <complex>

namespace dsp
{
    class AgcCC
    {
    private:
        float _rate;      // adjustment rate
        float _reference; // reference value
        float _gain;      // current gain
        float _max_gain;  // max allowable gain

        std::complex<float> scale(std::complex<float> input)
        {
            std::complex<float> output = input * _gain;

            _gain += _rate * (_reference - std::sqrt(output.real() * output.real() +
                                                     output.imag() * output.imag()));
            if (_max_gain > 0.0 && _gain > _max_gain)
            {
                _gain = _max_gain;
            }
            return output;
        }

        void scaleN(std::complex<float> output[], const std::complex<float> input[], unsigned n)
        {
            for (unsigned i = 0; i < n; i++)
            {
                output[i] = scale(input[i]);
            }
        }

    public:
        AgcCC(float rate, float reference, float gain, float max_gain);

        float rate() const { return _rate; }
        float reference() const { return _reference; }
        float gain() const { return _gain; }
        float max_gain() const { return _max_gain; }

        void set_rate(float rate) { _rate = rate; }
        void set_reference(float reference) { _reference = reference; }
        void set_gain(float gain) { _gain = gain; }
        void set_max_gain(float max_gain) { _max_gain = max_gain; }

        int work(std::complex<float> *in, int length, std::complex<float> *out);
    };
} // namespace libdsp