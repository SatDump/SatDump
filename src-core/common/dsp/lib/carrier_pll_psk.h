#pragma once
#include <complex>

namespace dsp
{
    class BPSKCarrierPLL
    {
    private:
        float d_alpha;      // 1st order loop constant
        float d_beta;       // 2nd order loop constant
        float d_max_offset; // Maximum frequency offset, radians/sample
        float d_phase;      // Instantaneous carrier phase
        float d_freq;       // Instantaneous carrier frequency, radians/sample
    public:
        BPSKCarrierPLL(float alpha, float beta, float max_offset);
        ~BPSKCarrierPLL();

        void set_alpha(float alpha) { d_alpha = alpha; }
        void set_beta(float beta) { d_beta = beta; }
        void set_max_offset(float max_offset) { d_max_offset = max_offset; }

        int work(std::complex<float> *in, int length, float *out);
    };
} // namespace libdsp