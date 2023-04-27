#pragma once

#include "common/dsp/block.h"
#include "common/dsp/resamp/polyphase_bank.h"

/*
Complex and real Gardner clock recovery.
*/
namespace dsp
{
    template <typename T>
    class GardnerClockRecoveryBlock : public Block<T, T>
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

        T sample;
        T zc_sample;
        T last_sample;

        int bufs;

        void work();

        PolyphaseBank pfb;

        float phase_error = 0;

        int ouc = 0;
        int inc = 0;

    public:
        GardnerClockRecoveryBlock(std::shared_ptr<dsp::stream<T>> input, float omega, float omegaGain, float mu, float muGain, float omegaLimit, int nfilt = 128, int ntaps = 8);
        ~GardnerClockRecoveryBlock();
    };
}