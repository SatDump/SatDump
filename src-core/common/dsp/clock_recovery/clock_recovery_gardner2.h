#pragma once

#include "common/dsp/block.h"
#include "common/dsp/resamp/polyphase_bank.h"

/*
Real Gardner clock recovery.
Mainly meant for FSK signals
*/
namespace dsp
{
    class GardnerClockRecovery2Block : public Block<float, float>
    {
    private:
        // Buffer
        // T *buffer;

        void work();

        // LPF Filter & Interpolator
        float *lpf_mem;
        float *lpf_coeffs;

        int lpf_size;
        int lpf_num_phases;
        int lpf_idx;

        // Timing
        float interm;
        float tim_prev;
        float tim_phase, tim_freq;
        float tim_alpha, tim_beta;
        float tim_max_fdev, tim_center_freq;
        int tim_state;

    public:
        GardnerClockRecovery2Block(std::shared_ptr<dsp::stream<float>> input,
                                   float samplerate, float symbolrate,
                                   float zeta, int order);
        ~GardnerClockRecovery2Block();
    };
}