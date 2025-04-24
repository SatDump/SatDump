#include "quadrature_demod.h"
#include "dsp/block_simple.h"
#include "dsp/flowgraph/flowgraph.h"
#include <complex.h>

namespace satdump
{
    namespace ndsp
    {
        QuadratureDemodBlock::QuadratureDemodBlock() : satdump::ndsp::BlockSimple<complex_t, float>("quadrature_demod_cf", {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"out", DSP_SAMPLE_TYPE_F32}}) {}

        QuadratureDemodBlock::~QuadratureDemodBlock() {}

        uint32_t QuadratureDemodBlock::process(complex_t *input, uint32_t nsamples, float *output)
        {
#if 0
        memcpy(&buffer[1], input, nsamples * sizeof(complex_t));

        volk_32fc_x2_multiply_conjugate_32fc((lv_32fc_t *)&buffer_out[0], (lv_32fc_t *)&buffer[1], (lv_32fc_t *)&buffer[0], nsamples);

        memmove(&buffer[nsamples], &buffer[1], 1 * sizeof(complex_t));

        for (int i = 0; i < nsamples; i++)
            output[i] = gain * fast_atan2f(buffer_out[i].imag, buffer_out[i].real);
#else
            for (uint32_t i = 0; i < nsamples; i++)
            {
                float p = atan2f(input[i].imag, input[i].real);
                float phase_diff = p - phase;

                if (phase_diff > M_PI)
                    phase_diff -= 2.0f * M_PI;
                else if (phase_diff <= -M_PI)
                    phase_diff += 2.0f * M_PI;

                output[i] = phase_diff * gain;
                phase = p;
            }
#endif

            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump