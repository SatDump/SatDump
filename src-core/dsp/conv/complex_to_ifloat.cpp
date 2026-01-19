#include "complex_to_ifloat.h"
#include "dsp/block.h"
#include "dsp/block_simple.h"
#include <complex.h>
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        ComplexToIFloatBlock::ComplexToIFloatBlock() : BlockSimple("complex_to_ifloat_cf", {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"out", DSP_SAMPLE_TYPE_F32}})
        {
            BlockSimple<complex_t, float>::output_buffer_size_ratio = 2;
        }

        ComplexToIFloatBlock::~ComplexToIFloatBlock() {}

        uint32_t ComplexToIFloatBlock::process(complex_t *input, uint32_t nsamples, float *output)
        {
            for (uint32_t i = 0; i < nsamples; i++)
            {
                output[i * 2 + 0] = input[i].real;
                output[i * 2 + 1] = input[i].imag;
            }

            return nsamples * 2;
        }
    } // namespace ndsp
} // namespace satdump