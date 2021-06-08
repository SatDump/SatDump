#pragma once

#include "block.h"

namespace dsp
{
    class FreqShiftBlock : public Block<std::complex<float>, std::complex<float>>
    {
    private:
        void work();
        std::complex<float> phase_delta;
        std::complex<float> phase;

    public:
        FreqShiftBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, float samplerate, float shift);
    };
}