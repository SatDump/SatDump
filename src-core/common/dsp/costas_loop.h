#pragma once

#include "block.h"

/*
Costas loop implementation 
with order 2, 4 and 8 support
*/
namespace dsp
{
    class CostasLoopBlock : public Block<complex_t, complex_t>
    {
    private:
        float error;
        float noise;
        int order;

        float phase, freq;
        float loop_bw;
        float alpha, beta;

        complex_t previous_vco;
        complex_t tmp_val;
        void work();

    public:
        CostasLoopBlock(std::shared_ptr<dsp::stream<complex_t>> input, float loop_bw, unsigned int order);
    };
}