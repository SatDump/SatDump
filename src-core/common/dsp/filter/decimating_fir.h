#pragma once

#include "common/dsp/block.h"
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
    template <typename T>
    class DecimatingFIRBlock : public Block<T, T>
    {
    private:
        float **taps;
        int ntaps;
        int align;
        int aligned_tap_count;

        int d_decimation;

        void work();

        int inc = 0, outc = 0;

        T *buffer;

    public:
        DecimatingFIRBlock(std::shared_ptr<dsp::stream<T>> input, std::vector<float> taps, int decimation);
        ~DecimatingFIRBlock();

        int process(T *input, int nsamples, T *output);
    };
}