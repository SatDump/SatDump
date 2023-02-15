#pragma once

#include "common/dsp/block.h"

/*
Delay the I branch by one sample.
This is meant mainly for OQPSK
demodulation.
*/
namespace dsp
{
    class DelayOneImagBlock : public Block<complex_t, complex_t>
    {
    private:
        float lastSamp;
        void work();

    public:
        DelayOneImagBlock(std::shared_ptr<dsp::stream<complex_t>> input);
    };
}