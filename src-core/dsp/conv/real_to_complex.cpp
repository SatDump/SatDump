#include "real_to_complex.h"

namespace satdump
{
    namespace ndsp
    {
        RealToComplexBlock::RealToComplexBlock() : BlockSimple<float, complex_t>("real_to_complex_fc", {{"in", DSP_SAMPLE_TYPE_F32}}, {{"out", DSP_SAMPLE_TYPE_CF32}}) {}

        RealToComplexBlock::~RealToComplexBlock() {}

        uint32_t RealToComplexBlock::process(float *input, uint32_t nsamples, complex_t *output)
        {
            for (uint32_t i = 0; i < nsamples; i++)
            {
                output[i] = input[i];
            }
            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump
