#include "hilbert.h"
#include <complex.h>

namespace satdump
{
    namespace ndsp
    {

        HilbertBlock::HilbertBlock() : Block("hilber_fc", {{"in", DSP_SAMPLE_TYPE_F32}}, {{"out", DSP_SAMPLE_TYPE_CF32}}) {}

        HilbertBlock::~HilbertBlock()
        {
            if (taps != nullptr)
                volk_free(taps);

            if (buffer != nullptr)
                volk_free(buffer);
        }

        bool HilbertBlock::work()
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

            float *ibuf = iblk.getSamples<float>();

            lbuf_size = iblk.size > buffer_size ? buffer_size : iblk.size;
            lbuf_offset = 0;

            while (lbuf_offset < iblk.size)
            {
                DSPBuffer oblk = DSPBuffer::newBufferSamples<complex_t>(lbuf_size);
                complex_t *obuf = oblk.getSamples<complex_t>();

                memcpy(&buffer[ntaps], ibuf + lbuf_offset, lbuf_size * sizeof(float));

                for (int i = 0; i < lbuf_size; i++)
                {
                    volk_32f_x2_dot_prod_32f(&obuf[i].imag, &buffer[i], taps, ntaps);
                    obuf[i].real = buffer[i + ntaps / 2];
                }

                oblk.size = lbuf_size;
                outputs[0].fifo->wait_enqueue(oblk);

                memmove(&buffer[0], &buffer[lbuf_size], ntaps * sizeof(float));
                lbuf_offset += lbuf_size;
            }

            iblk.free();

            return false;
        }
    } // namespace ndsp
} // namespace satdump