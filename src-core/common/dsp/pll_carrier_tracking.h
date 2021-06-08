#pragma once

#include "block.h"
#include "lib/pll_carrier_tracking.h"

namespace dsp
{
    class PLLCarrierTrackingBlock : public Block<std::complex<float>, std::complex<float>>
    {
    private:
        dsp::PLLCarrierTracking d_pll;
        void work();

    public:
        PLLCarrierTrackingBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, float loop_bw, unsigned int min, unsigned int max);
    };
}