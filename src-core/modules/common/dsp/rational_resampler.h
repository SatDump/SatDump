#pragma once

#include "block.h"
#include "lib/rational_resampler.h"

namespace dsp
{
    class CCRationalResamplerBlock : public Block<std::complex<float>, std::complex<float>>
    {
    private:
        dsp::RationalResamplerCCF d_res;
        void work();

    public:
        CCRationalResamplerBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, unsigned interpolation, unsigned decimation);
    };
}