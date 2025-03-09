#include "fir.h"
#include "common/dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        FIRBlock<T>::FIRBlock()
            : Block(std::is_same_v<T, complex_t> ? "fir_cc" : "fir_ff",
                    {{"in", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_COMPLEX : DSP_SAMPLE_TYPE_FLOAT}},
                    {{"out", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_COMPLEX : DSP_SAMPLE_TYPE_FLOAT}})
        {
            outputs[0].fifo = std::make_shared<moodycamel::BlockingReaderWriterCircularBuffer<DSPBuffer>>(16);

            // Init buffer
            buffer = dsp::create_volk_buffer<T>(2 * 1e6); // TODOREWORK How to handle this from the initial buffer size?
        }

        template <typename T>
        FIRBlock<T>::~FIRBlock()
        {
            if (taps != nullptr)
            {
                for (int i = 0; i < aligned_tap_count; i++)
                    volk_free(taps[i]);
                volk_free(taps);
            }

            if (buffer != nullptr)
                volk_free(buffer);
        }

        template <typename T>
        bool FIRBlock<T>::work()
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

            int nsamples = iblk.size;
            DSPBuffer oblk = DSPBuffer::newBufferSamples<T>(iblk.max_size);

            T *ibuf = iblk.getSamples<T>();
            T *obuf = oblk.getSamples<T>();

            memcpy(&buffer[ntaps], ibuf, nsamples * sizeof(T));
            iblk.free();

            if constexpr (std::is_same_v<T, float>)
            {
                for (int i = 0; i < nsamples; i++)
                {
                    // Doing it this way instead of the normal :
                    // volk_32f_x2_dot_prod_32f(&output_stream->writeBuf[i], &buffer[i + 1], taps, ntaps);
                    // turns out to enhance performances quite a bit... Initially tested this after noticing
                    // inconsistencies between the above simple way and GNU Radio's implementation
                    const float *ar = (float *)((size_t)&buffer[i + 1] & ~(align - 1));
                    const unsigned al = &buffer[i + 1] - ar;
                    volk_32f_x2_dot_prod_32f_a(&obuf[i], ar, taps[al], ntaps + al);
                }
            }
            if constexpr (std::is_same_v<T, complex_t>)
            {
                for (int i = 0; i < nsamples; i++)
                {
                    // Doing it this way instead of the normal :
                    // volk_32fc_32f_dot_prod_32fc(&output_stream->writeBuf[i], &buffer[i + 1], taps, ntaps);
                    // turns out to enhance performances quite a bit... Initially tested this after noticing
                    // inconsistencies between the above simple way and GNU Radio's implementation
                    const complex_t *ar = (complex_t *)((size_t)&buffer[i + 1] & ~(align - 1));
                    const unsigned al = &buffer[i + 1] - ar;
                    volk_32fc_32f_dot_prod_32fc_a((lv_32fc_t *)&obuf[i], (lv_32fc_t *)ar, taps[al], ntaps + al);
                }
            }

            oblk.size = nsamples;
            outputs[0].fifo->wait_enqueue(oblk);

            memmove(&buffer[0], &buffer[nsamples], ntaps * sizeof(T));

            return false;
        }

        template class FIRBlock<complex_t>;
        template class FIRBlock<float>;
    }
}