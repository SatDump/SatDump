#include "delay_one_imag.h"

namespace satdump
{
    namespace ndsp
    {
        DelayOneImagBlock::DelayOneImagBlock() : Block("delay_one_imag_cc", {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"out", DSP_SAMPLE_TYPE_CF32}}) {}

        DelayOneImagBlock::~DelayOneImagBlock() {}

        bool DelayOneImagBlock::work()
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

            auto oblk = DSPBuffer::newBufferSamples<complex_t>(iblk.max_size);
            complex_t *ibuf = iblk.getSamples<complex_t>();
            complex_t *obuf = iblk.getSamples<complex_t>();

            for (uint32_t i = 0; i < iblk.size; i++)
            {
                float imag = i == 0 ? lastSamp : ibuf[i - 1].imag;
                float real = ibuf[i].real;
                obuf[i] = complex_t(real, imag);
            }

            lastSamp = ibuf[iblk.size - 1].imag;

            oblk.size = iblk.size;
            outputs[0].fifo->wait_enqueue(oblk);
            iblk.free();
            return false;
        }

    }; // namespace ndsp

} // namespace satdump
