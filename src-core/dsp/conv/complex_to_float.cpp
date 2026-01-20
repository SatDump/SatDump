#include "complex_to_float.h"
#include "dsp/block.h"
#include "dsp/block_simple.h"
#include <complex.h>
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        ComplexToFloatBlock::ComplexToFloatBlock() : BlockSimpleMulti("complex_to_float_cf", {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"real", DSP_SAMPLE_TYPE_F32}, {"imag", DSP_SAMPLE_TYPE_F32}}) {}

        ComplexToFloatBlock::~ComplexToFloatBlock() {}

        void ComplexToFloatBlock::process(complex_t **input, uint32_t *nsamples, float **output, uint32_t *nsamples_out)
        {
            for (uint32_t i = 0; i < nsamples[0]; i++)
            {
                output[0][i] = input[i]->real;
                output[1][i] = input[i]->imag;
            }

            nsamples_out[0] = nsamples[0];
            nsamples_out[1] = nsamples[0];
        }
    } // namespace ndsp
} // namespace satdump