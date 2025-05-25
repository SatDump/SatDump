#include "freq_shift.h"

namespace satdump
{
    namespace ndsp
    {
        FreqShiftBlock::FreqShiftBlock() : BlockSimple<complex_t, complex_t>("freq_shift_cc", {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"out", DSP_SAMPLE_TYPE_CF32}}) {}

        FreqShiftBlock::~FreqShiftBlock() {}

        uint32_t FreqShiftBlock::process(complex_t *input, uint32_t nsamples, complex_t *output)
        {

            if (needs_reinit)
            {
                needs_reinit = false;
                init();
            }

#if VOLK_VERSION >= 030100
            volk_32fc_s32fc_x2_rotator2_32fc((lv_32fc_t *)output, (const lv_32fc_t *)input, (lv_32fc_t *)&phase_delta, (lv_32fc_t *)&phase, nsamples);
#else
            volk_32fc_s32fc_x2_rotator_32fc((lv_32fc_t *)output, (const lv_32fc_t *)input, phase_delta, (lv_32fc_t *)&phase, nsamples);
#endif

            return nsamples;
        }

    } // namespace ndsp
} // namespace satdump
