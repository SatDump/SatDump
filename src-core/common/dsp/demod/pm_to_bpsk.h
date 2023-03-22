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
        complex_t phase_delta, phase;

        void work();

    public:
        PMToBPSK(std::shared_ptr<dsp::stream<complex_t>> input, float samplerate, float symbolrate);
    };
}