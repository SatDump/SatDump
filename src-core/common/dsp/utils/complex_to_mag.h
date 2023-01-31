#pragma once

#include "common/dsp/block.h"

/*
Amplitude demodulator.
Used for AM/ASK
*/
namespace dsp
{
    class ComplexToMagBlock : public Block<complex_t, float>
    {
    private:
        void work();

    public:
        ComplexToMagBlock(std::shared_ptr<dsp::stream<complex_t>> input);
        ~ComplexToMagBlock();
    };
}