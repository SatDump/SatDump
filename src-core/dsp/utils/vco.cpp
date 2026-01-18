#include "vco.h"
#include <complex.h>

namespace satdump
{
    namespace ndsp
    {
        VCOBlock::VCOBlock() : BlockSimple<float, complex_t>("vco_fc", {{"in", DSP_SAMPLE_TYPE_F32}}, {{"out", DSP_SAMPLE_TYPE_CF32}}) {}

        VCOBlock::~VCOBlock() {}

        uint32_t VCOBlock::process(float *input, uint32_t nsamples, complex_t *output)
        {
            for (uint32_t i = 0; i < nsamples; i++)
            {
                output[i].real = cosf(d_phase) * d_amp;
                output[i].imag = sinf(d_phase) * d_amp;
                d_phase += input[i] * d_k;

                // Wrap phase
                while (d_phase > (2 * M_PI))
                    d_phase -= 2 * M_PI;
                while (d_phase < (-2 * M_PI))
                    d_phase += 2 * M_PI;
            }

            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump