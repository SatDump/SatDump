#pragma once

#include "block.h"
#include "lib/dc_blocker.h"

namespace dsp
{
    class DCBlockerBlock : public Block<std::complex<float>, std::complex<float>>
    {
    private:
        dsp::DCBlocker d_blocker;
        void work();

    public:
        DCBlockerBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, int length, bool long_form);
    };
}