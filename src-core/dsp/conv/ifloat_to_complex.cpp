#include "ifloat_to_complex.h"
#include "dsp/block.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        IFloatToComplexBlock::IFloatToComplexBlock() : BlockSimple("ifloat_to_complex_fc", {{"in", DSP_SAMPLE_TYPE_F32}}, {{"out", DSP_SAMPLE_TYPE_CF32}}) {}

        IFloatToComplexBlock::~IFloatToComplexBlock() {}

        uint32_t IFloatToComplexBlock::process(float *input, uint32_t nsamples, complex_t *output)
        {
            uint32_t nout = 0;

            for (int i = 0; i < nsamples; i++, state++)
            {
                if (state % 2 == 0)
                {
                    wip_c.real = input[i];
                }
                else if (state % 2 == 1)
                {
                    wip_c.imag = input[i];
                    output[nout++] = wip_c;
                }
            }

            return nout;
        }
    } // namespace ndsp
} // namespace satdump