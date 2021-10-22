#pragma once

#include "block.h"

/*
Complex and real clock recovery M&M.
This is like 0.1% slower than using  aligned taps,
but leads to much greater stability and output.
*/
namespace dsp
{
    class CCMMClockRecoveryBlock : public Block<complex_t, complex_t>
    {
    private:
        // Buffer
        complex_t *buffer;
        int in_buffer;

        // Variables
        float mu;
        float omega;
        float omega_gain;
        float mu_gain;
        float omega_relative_limit;
        float omega_mid;
        float omega_limit;

        complex_t p_2T, p_1T, p_0T;
        complex_t c_2T, c_1T, c_0T;

        void work();

    public:
        CCMMClockRecoveryBlock(std::shared_ptr<dsp::stream<complex_t>> input, float omega, float omegaGain, float mu, float muGain, float omegaLimit);
        ~CCMMClockRecoveryBlock();
    };

    class FFMMClockRecoveryBlock : public Block<float, float>
    {
    private:
        // Buffer
        float *buffer;
        int in_buffer;

        // Variables
        float mu;
        float omega;
        float omega_gain;
        float mu_gain;
        float omega_relative_limit;
        float omega_mid;
        float omega_limit;

        float last_sample;

        void work();

    public:
        FFMMClockRecoveryBlock(std::shared_ptr<dsp::stream<float>> input, float omega, float omegaGain, float mu, float muGain, float omegaLimit);
        ~FFMMClockRecoveryBlock();
    };
}