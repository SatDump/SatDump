#pragma once

#include "common/dsp/block.h"

/*
Frequency shifer
*/
namespace dsp
{
    class FreqShiftBlock : public Block<complex_t, complex_t>
    {
    private:
        void work();
        std::mutex mtx;
        complex_t phase_delta;
        complex_t phase;

    public:
        FreqShiftBlock(std::shared_ptr<dsp::stream<complex_t>> input, double samplerate, double shift);

        void set_freq(double samplerate, double freq);
        void set_freq_raw(double freq); // Allows using this as a manual frequency correction block, eg, as a pre-PLL tool
    };
}