#include "agc.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        AGCBlock<T>::AGCBlock()
            : Block(std::is_same_v<T, complex_t> ? "agc_cc" : "agc_ff",
                    {{"in", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}},
                    {{"out", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}})
        {
        }

        template <typename T>
        AGCBlock<T>::~AGCBlock()
        {
        }

        template <typename T>
        bool AGCBlock<T>::work()
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

            DSPBuffer oblk = DSPBuffer::newBufferSamples<T>(iblk.max_size);
            T *ibuf = iblk.getSamples<T>();
            T *obuf = oblk.getSamples<T>();

            for (uint32_t i = 0; i < iblk.size; i++)
            {
                T output = ibuf[i] * gain;

                if constexpr (std::is_same_v<T, float>)
                    gain += rate * (reference - fabsf(output));
                if constexpr (std::is_same_v<T, complex_t>)
                    gain += rate * (reference - sqrt(output.real * output.real +
                                                     output.imag * output.imag));

                if (max_gain > 0.0 && gain > max_gain)
                    gain = max_gain;

                obuf[i] = output;
            }

            oblk.size = iblk.size;
            outputs[0].fifo->wait_enqueue(oblk);
            iblk.free();

            return false;
        }

        template class AGCBlock<complex_t>;
        template class AGCBlock<float>;
    }
}