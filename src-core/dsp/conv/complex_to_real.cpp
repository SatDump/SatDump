#include "complex_to_real.h"

namespace satdump
{
    namespace ndsp
    {
        ComplexToRealBlock::ComplexToRealBlock() : BlockSimple<complex_t, float>("complex_to_real_cf", {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"out", DSP_SAMPLE_TYPE_F32}}) {}

        ComplexToRealBlock::~ComplexToRealBlock() {}

        uint32_t ComplexToRealBlock::process(complex_t *input, uint32_t nsamples, float *output)
        {
            for (uint32_t i = 0; i < nsamples; i++)
            {
                output[i] = input[i].real;
            }
            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump
