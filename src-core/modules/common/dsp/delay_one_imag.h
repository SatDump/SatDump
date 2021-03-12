#pragma once

#include "block.h"
#include "modules/common/delay_one_imag.h"

namespace dsp
{
    class DelayOneImagBlock : public Block<std::complex<float>, std::complex<float>>
    {
    private:
        DelayOneImag d_delay;
        void work();

    public:
        DelayOneImagBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input);
    };
}