#pragma once

#include "block.h"
#include "lib/agc.h"

namespace dsp
{
    class AGCBlock : public Block<std::complex<float>, std::complex<float>>
    {
    private:
        dsp::AgcCC d_agc;
        void work();

    public:
        AGCBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, float agc_rate, float reference, float gain, float max_gain);
    };
}