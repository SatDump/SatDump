#include "float_to_complex.h"
#include "dsp/block.h"
#include "dsp/block_simple.h"
#include <complex.h>
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        FloatToComplexBlock::FloatToComplexBlock() : BlockSimpleMulti("float_to_complex_fc", {{"real", DSP_SAMPLE_TYPE_F32}, {"imag", DSP_SAMPLE_TYPE_F32}}, {{"out", DSP_SAMPLE_TYPE_CF32}}) {}

        FloatToComplexBlock::~FloatToComplexBlock() {}

        void FloatToComplexBlock::process(float **input, uint32_t *nsamples, complex_t **output, uint32_t *nsamples_out)
        {
            if (nsamples[0] != nsamples[1])
            {
                nsamples_out[0] = 0;
                return;
            }

            for (uint32_t i = 0; i < nsamples[0]; i++)
            {
                output[0][i].real = input[0][i];
                output[0][i].imag = input[1][i];
            }

            nsamples_out[0] = nsamples[0];
        }
    } // namespace ndsp
} // namespace satdump