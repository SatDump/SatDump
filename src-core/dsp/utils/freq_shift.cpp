#include "freq_shift.h"

namespace satdump
{
    namespace ndsp
    {
        FreqShiftBlock::FreqShiftBlock() : Block("freq_shift_cc", {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"out", DSP_SAMPLE_TYPE_CF32}}) {}

        FreqShiftBlock::~FreqShiftBlock() {}

        bool FreqShiftBlock::work()
        {
            DSPBuffer iblk;
            inputs[0].fifo->wait_dequeue(iblk);

            if (iblk.isTerminator())
            {
                if (iblk.terminatorShouldPropagate())
                    outputs[0].fifo->wait_enqueue(DSPBuffer::newBufferTerminator());
                iblk.free();
                return true;
            }

            if (needs_reinit)
            {
                needs_reinit = false;
                init();
            }

            auto oblk = DSPBuffer::newBufferSamples<complex_t>(iblk.max_size);
            complex_t *ibuf = iblk.getSamples<complex_t>();
            complex_t *obuf = oblk.getSamples<complex_t>();

            volk_32fc_s32fc_x2_rotator2_32fc((lv_32fc_t *)obuf, (const lv_32fc_t *)ibuf, (lv_32fc_t *)&phase_delta, (lv_32fc_t *)&phase, iblk.size);

            oblk.size = iblk.size;
            outputs[0].fifo->wait_enqueue(oblk);
            iblk.free();

            return false;
        }

    } // namespace ndsp
} // namespace satdump
