#pragma once

#include "common/dsp/block.h"

/*
Real To Complex
*/
namespace dsp
{
    class RealToComplexBlock : public Block<float, complex_t>
    {
    private:
        void work();

    public:
        RealToComplexBlock(std::shared_ptr<dsp::stream<float>> input);
        ~RealToComplexBlock();
    };
}