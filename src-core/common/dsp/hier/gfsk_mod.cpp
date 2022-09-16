#include "gfsk_mod.h"

namespace dsp
{
    GFSKMod::GFSKMod(std::shared_ptr<dsp::stream<float>> input, float sensitivity, float alpha, float taps)
        : HierBlock(input)
    {
        fir_shaping = std::make_shared<dsp::RationalResamplerBlock<float>>(input_stream, 2, 1, dsp::firdes::convolve(dsp::firdes::gaussian(1, 2, alpha, taps), {1, 1}));
        vco_gen = std::make_shared<dsp::VCOBlock>(fir_shaping->output_stream, sensitivity, 1.0);
        output_stream = vco_gen->output_stream;
    }

    void GFSKMod::start()
    {
        fir_shaping->start();
        vco_gen->start();
    }

    void GFSKMod::stop()
    {
        fir_shaping->stop();
        vco_gen->stop();
    }
}