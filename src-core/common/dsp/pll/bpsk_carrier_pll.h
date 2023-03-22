#pragma once

#include "common/dsp/block.h"

namespace dsp
{
    class BPSKCarrierPLLBlock : public Block<complex_t, float>
    {
    private:
        float alpha;      // 1st order loop constant
        float beta;       // 2nd order loop constant
        float max_offset; // Maximum frequency offset, radians/sample
        float phase = 0;  // Instantaneous carrier phase
        float freq = 0;   // Instantaneous carrier frequency, radians/sample

        void work();

    public:
        BPSKCarrierPLLBlock(std::shared_ptr<dsp::stream<complex_t>> input, float alpha, float beta, float max_offset);
    };
}