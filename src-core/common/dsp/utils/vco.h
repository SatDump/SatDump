#pragma once

#include "common/dsp/block.h"

/*
VCO for FM/FSK
Modulation
*/
namespace dsp
{
    class VCOBlock : public Block<float, complex_t>
    {
    private:
        void work();
        float d_k;
        float d_amp;
        float d_phase = 0;

    public:
        VCOBlock(std::shared_ptr<dsp::stream<float>> input, float k, float amp);
    };
}