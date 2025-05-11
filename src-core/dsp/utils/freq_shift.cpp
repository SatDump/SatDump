#include "freq_shift.h"
#include <cmath>

// TODOREWORK? benchmark the speed of current implementation vs using memcpy (like inside #if 0 in "quadrature_demod.cpp")

namespace satdump
{
    namespace ndsp
    {
        FreqShiftBlock::FreqShiftBlock() : satdump::ndsp::BlockSimple<complex_t, complex_t>("freq_shift_cc", {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"out", DSP_SAMPLE_TYPE_CF32}})
        {
            if (raw_shift)
            {
                phase_delta = complex_t(cosf(freq_shift), sinf(freq_shift));
            }
            else
            {
                phase = complex_t(1, 0);
                phase_delta = complex_t(cos(2.0 * M_PI * (freq_shift / samplerate)), sin(2.0 * M_PI * (freq_shift / samplerate)));
            }
        }

        FreqShiftBlock::~FreqShiftBlock() {}

        uint32_t FreqShiftBlock::process(complex_t *input, uint32_t nsamples, complex_t *output)
        {
            for (uint32_t i = 0; i < nsamples; i++)
            {
                volk_32fc_s32fc_x2_rotator2_32fc((lv_32fc_t *)&output[i], (lv_32fc_t *)&input[i], (lv_32fc_t *)&phase_delta, (lv_32fc_t *)&phase, nsamples);
            }

            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump
