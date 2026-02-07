#include "complex_to_imag.h"

namespace satdump
{
    namespace ndsp
    {
        ComplexToImagBlock::ComplexToImagBlock() : BlockSimple<complex_t, float>("complex_to_imag_cf", {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"out", DSP_SAMPLE_TYPE_F32}}) {}

        ComplexToImagBlock::~ComplexToImagBlock() {}

        uint32_t ComplexToImagBlock::process(complex_t *input, uint32_t nsamples, float *output)
        {
            for (uint32_t i = 0; i < nsamples; i++)
            {
                output[i] = input[i].imag;
            }
            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump
