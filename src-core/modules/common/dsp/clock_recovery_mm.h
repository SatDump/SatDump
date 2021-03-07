#pragma once

#include "mmse_fir_interpolator.h"

namespace dsp
{
    class ClockRecoveryMMCC
    {
    public:
        // Init clock recovery with default values
        ClockRecoveryMMCC(float samplerate, float symbolrate);
        // Init clock recovery with custom values
        ClockRecoveryMMCC(float omega, float omegaGain, float mu, float muGain, float omegaLimit);
        ~ClockRecoveryMMCC();

        int work(std::complex<float> *in, int length, std::complex<float> *out);

        float mu() const { return d_mu; }
        float omega() const { return d_omega; }
        float gain_mu() const { return d_gain_mu; }
        float gain_omega() const { return d_gain_omega; }

        void set_verbose(bool verbose) { d_verbose = verbose; }
        void set_gain_mu(float gain_mu) { d_gain_mu = gain_mu; }
        void set_gain_omega(float gain_omega) { d_gain_omega = gain_omega; }
        void set_mu(float mu) { d_mu = mu; }
        void set_omega(float omega);

    private:
        float d_mu;                   // fractional sample position [0.0, 1.0]
        float d_omega;                // nominal frequency
        float d_gain_omega;           // gain for adjusting omega
        float d_omega_relative_limit; // used to compute min and max omega
        float d_omega_mid;            // average omega
        float d_omega_lim;            // actual omega clipping limit
        float d_gain_mu;              // gain for adjusting mu

        std::complex<float> d_last_sample;
        mmse_fir_interpolator_cc d_interp;

        bool d_verbose;

        std::complex<float> d_p_2T, d_p_1T, d_p_0T;
        std::complex<float> d_c_2T, d_c_1T, d_c_0T;

        std::complex<float> slicer_0deg(std::complex<float> sample)
        {
            float real = 0.0f, imag = 0.0f;

            if (sample.real() > 0.0f)
                real = 1.0f;
            if (sample.imag() > 0.0f)
                imag = 1.0f;
            return std::complex<float>(real, imag);
        }

        std::complex<float> slicer_45deg(std::complex<float> sample)
        {
            float real = -1.0f, imag = -1.0f;
            if (sample.real() > 0.0f)
                real = 1.0f;
            if (sample.imag() > 0.0f)
                imag = 1.0f;
            return std::complex<float>(real, imag);
        }

        // History needed!
        volk::vector<std::complex<float>> tmp_in;
    };

    class ClockRecoveryMMFF
    {
    public:
        // Init clock recovery with default values
        //ClockRecovery(float samplerate, float symbolrate);
        // Init clock recovery with custom values
        ClockRecoveryMMFF(float omega, float omegaGain, float mu, float muGain, float omegaLimit);
        ~ClockRecoveryMMFF();

        int work(float *in, int length, float *out);

        float mu() const { return d_mu; }
        float omega() const { return d_omega; }
        float gain_mu() const { return d_gain_mu; }
        float gain_omega() const { return d_gain_omega; }

        void set_gain_mu(float gain_mu) { d_gain_mu = gain_mu; }
        void set_gain_omega(float gain_omega) { d_gain_omega = gain_omega; }
        void set_mu(float mu) { d_mu = mu; }
        void set_omega(float omega);

    private:
        float d_mu;                   // fractional sample position [0.0, 1.0]
        float d_gain_mu;              // gain for adjusting mu
        float d_omega;                // nominal frequency
        float d_gain_omega;           // gain for adjusting omega
        float d_omega_relative_limit; // used to compute min and max omega
        float d_omega_mid;            // average omega
        float d_omega_lim;            // actual omega clipping limit

        float d_last_sample;
        mmse_fir_interpolator_ff d_interp;

        float slice(float x) { return x < 0 ? -1.0F : 1.0F; }

        // History needed!
        volk::vector<float> tmp_in;
    };
} // namespace libdsp