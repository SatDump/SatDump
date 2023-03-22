#pragma once

#include "common/dsp/block.h"
#include "common/dsp/resamp/polyphase_bank.h"

/*
Complex and real clock recovery M&M.
This is like 0.1% slower than using  aligned taps,
but leads to much greater stability and output.
*/
namespace dsp
{
    template <typename T>
    class MMClockRecoveryBlock : public Block<T, T>
    {
    private:
        // Buffer
        T *buffer;

        // Variables
        float mu;
        float omega;
        float omega_gain;
        float mu_gain;
        float omega_relative_limit;
        float omega_mid;
        float omega_limit;

        float sample;
        float last_sample;

        complex_t p_2T, p_1T, p_0T;
        complex_t c_2T, c_1T, c_0T;

        void work();

        PolyphaseBank pfb;

        float phase_error = 0;

        int ouc = 0;
        int inc = 0;

    public:
        MMClockRecoveryBlock(std::shared_ptr<dsp::stream<T>> input, float omega, float omegaGain, float mu, float muGain, float omegaLimit, int nfilt = 128, int ntaps = 8);
        ~MMClockRecoveryBlock();
    };
}