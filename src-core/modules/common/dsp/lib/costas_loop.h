#pragma once
#include <complex>
#include <vector>
#include "control_loop.h"

namespace dsp
{
    class CostasLoop : public ControlLoop
    {
    private:
        float d_error;
        float d_noise;
        bool d_use_snr;
        int d_order;
        float phase_detector_8(std::complex<float> sample) const // for 8PSK
        {
            const float K = (sqrtf(2.0) - 1);
            if (fabsf(sample.real()) >= fabsf(sample.imag()))
            {
                return ((sample.real() > 0.0f ? 1.0f : -1.0f) * sample.imag() -
                        (sample.imag() > 0.0f ? 1.0f : -1.0f) * sample.real() * K);
            }
            else
            {
                return ((sample.real() > 0.0f ? 1.0f : -1.0f) * sample.imag() * K -
                        (sample.imag() > 0.0f ? 1.0f : -1.0f) * sample.real());
            }
        };

        float phase_detector_4(std::complex<float> sample) const // for QPSK
        {
            return ((sample.real() > 0.0f ? 1.0f : -1.0f) * sample.imag() -
                    (sample.imag() > 0.0f ? 1.0f : -1.0f) * sample.real());
        };

        float phase_detector_2(std::complex<float> sample) const // for BPSK
        {
            return (sample.real() * sample.imag());
        }

    public:
        CostasLoop(float loop_bw, unsigned int order);
        int work(std::complex<float> *in, int length, std::complex<float> *out);
    };
} // namespace libdsp