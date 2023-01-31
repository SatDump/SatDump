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
    class FIRBlock : public Block<T, T>
    {
    private:
        T *buffer;
        float **taps;
        int ntaps;
        int align;
        int aligned_tap_count;
        void work();

    public:
        FIRBlock(std::shared_ptr<dsp::stream<T>> input, std::vector<float> taps);
        ~FIRBlock();
    };
}