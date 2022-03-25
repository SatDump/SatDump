#pragma once

#include "block.h"

/*
Complex and real clock recovery M&M.
This is like 0.1% slower than using  aligned taps,
but leads to much greater stability and output.
*/
namespace dsp
{
    class GardnerClockRecoveryBlock : public Block<complex_t, complex_t>
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

        complex_t current_sample;
        complex_t zero_crossing_sample;
        complex_t last_sample;

        void work();

    public:
        GardnerClockRecoveryBlock(std::shared_ptr<dsp::stream<complex_t>> input, float omega, float omegaGain, float mu, float muGain, float omegaLimit);
        ~GardnerClockRecoveryBlock();
    };
}