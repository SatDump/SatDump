#pragma once

#include "block.h"

/*
Simple carrier tracking PLL, which also does 
PM Conversion to BPSK
*/
namespace dsp
{
    class PhaseModulationPLL : public Block<complex_t, complex_t>
    {
    private:
        float alpha;                             // 1st order loop constant
        float beta;                              // 2nd order loop constant
        float max_offset;                        // Maximum frequency offset, radians/sample
        float phase_carrier = 0, phase_real = 0; // Instantaneous carrier phase
        float freq = 0;                          // Instantaneous carrier frequency, radians/sample
        float phase_shift = 0;                   // Static phase increase

        // Work variables
        complex_t vco;
        complex_t shf;

        void work();

    public:
        PhaseModulationPLL(std::shared_ptr<dsp::stream<complex_t>> input, float loop_bw, float max_offset, float samplerate, float symbolrate);
    };
}