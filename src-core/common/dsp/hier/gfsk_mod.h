#pragma once

#include "block.h"
#include "common/dsp/vco.h"
#include "common/dsp/firdes.h"
#include "common/dsp/rational_resampler.h"

namespace dsp
{
    /*
    GFSK & GMSK modulator.
    For GMSK, use a M_PI / 4.0 as a sensitivity value.

    Baseband will be produce at 2 samples per symbols.

    Normally float input should be between -1 and 1.
    */
    class GFSKMod : public HierBlock<float, complex_t>
    {
    private:
        std::shared_ptr<dsp::FFRationalResamplerBlock> fir_shaping;
        std::shared_ptr<dsp::VCOBlock> vco_gen;

    public:
        GFSKMod(std::shared_ptr<dsp::stream<complex_t>> input, float sensitivity, float alpha, float taps = 31);
        void start();
        void stop();
    };
}