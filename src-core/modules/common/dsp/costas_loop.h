#pragma once

#include "block.h"
#include "lib/costas_loop.h"

namespace dsp
{
    class CostasLoopBlock : public Block<std::complex<float>, std::complex<float>>
    {
    private:
        dsp::CostasLoop d_costas;
        void work();

    public:
        CostasLoopBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, float loop_bw, unsigned int order);
    };
}