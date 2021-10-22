#pragma once

#include "block.h"
#include <vector>

/*
Basic FIR filters implementations.
This originally used GNU Radio's implementation,
but was now simplified down to what's actually necessary...
Decimation support was removed, as it is not currently
required anywhere.
*/
namespace dsp
{
    class CCFIRBlock : public Block<complex_t, complex_t>
    {
    private:
        complex_t *history;
        float **taps;
        int ntaps;
        int align;
        int aligned_tap_count;
        void work();

    public:
        CCFIRBlock(std::shared_ptr<dsp::stream<complex_t>> input, std::vector<float> taps);
        ~CCFIRBlock();
    };

    class FFFIRBlock : public Block<float, float>
    {
    private:
        float *history;
        float **taps;
        int ntaps;
        int align;
        int aligned_tap_count;
        void work();

    public:
        FFFIRBlock(std::shared_ptr<dsp::stream<float>> input, std::vector<float> taps);
        ~FFFIRBlock();
    };
}