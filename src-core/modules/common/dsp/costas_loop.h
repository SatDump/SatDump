#pragma once

#include "block.h"
#include <dsp/costas_loop.h>

namespace dsp
{
    class CostasLoopBlock : public Block<std::complex<float>, std::complex<float>>
    {
    private:
        libdsp::CostasLoop d_costas;
        void work();

    public:
        CostasLoopBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, float loop_bw, unsigned int order);
    };
}