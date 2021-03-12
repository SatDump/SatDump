#pragma once

#include "block.h"
#include "modules/common/dsp/fir_filter.h"

namespace dsp
{
    class CCFIRBlock : public Block<std::complex<float>, std::complex<float>>
    {
    private:
        dsp::FIRFilterCCF d_fir;
        void work();

    public:
        CCFIRBlock(dsp::stream<std::complex<float>> &input, int decimation, std::vector<float> taps);
    };

    class FFFIRBlock : public Block<float, float>
    {
    private:
        dsp::FIRFilterFFF d_fir;
        void work();

    public:
        FFFIRBlock(dsp::stream<float> &input, int decimation, std::vector<float> taps);
    };
}