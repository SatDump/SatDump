#pragma once

#include "block.h"
#include <dsp/dc_blocker.h>

namespace dsp
{
    class DCBlockerBlock : public Block<std::complex<float>, std::complex<float>>
    {
    private:
        libdsp::DCBlocker d_blocker;
        void work();

    public:
        DCBlockerBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, int length, bool long_form);
    };
}