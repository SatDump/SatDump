#pragma once

#include "common/dsp/block.h"

/*
Converts and shift down a PM signal to BPSK
*/
namespace dsp
{
    class PMToBPSK : public Block<complex_t, complex_t>
    {
    private:
        float phase_real = 0;
        float phase_shift = 0;

        // Work variables
        complex_t shf;

        void work();

    public:
        PMToBPSK(std::shared_ptr<dsp::stream<complex_t>> input, float samplerate, float symbolrate);
    };
}