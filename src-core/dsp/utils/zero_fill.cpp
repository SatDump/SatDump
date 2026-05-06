#include "zero_fill.h"
#include "dsp/block_helpers.h"
#include <complex.h>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        ZeroFillBlock<T>::ZeroFillBlock() : Block("zero_fill_" + getShortTypeName<T>() + getShortTypeName<T>(), {{"in", getTypeSampleType<T>()}}, {{"out", getTypeSampleType<T>()}})
        {
        }

        template <typename T>
        ZeroFillBlock<T>::~ZeroFillBlock()
        {
        }

        template <typename T>
        bool ZeroFillBlock<T>::work()
        {
            DSPBuffer iblk;
            if (inputs[0].fifo->try_dequeue(iblk))
            {
                if (iblk.isTerminator())
                {
                    if (iblk.terminatorShouldPropagate())
                        outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                    inputs[0].fifo->free(iblk);
                    return true;
                }

                DSPBuffer oblk = outputs[0].fifo->newBufferSamples(iblk.size, sizeof(T));
                T *ibuf = iblk.getSamples<T>();
                T *obuf = oblk.getSamples<T>();

                memcpy(obuf, ibuf, sizeof(T) * iblk.size);

                oblk.size = iblk.size;
                outputs[0].fifo->wait_enqueue(oblk);
                inputs[0].fifo->free(iblk);
            }
            else
            {
                const int lsize = zero_buffer_size;

                DSPBuffer oblk = outputs[0].fifo->newBufferSamples(lsize, sizeof(T));
                T *obuf = oblk.getSamples<T>();
                for (int i = 0; i < lsize; i++)
                    obuf[i] = 0;
                oblk.size = lsize;
                outputs[0].fifo->wait_enqueue(oblk);
            }

            return false;
        }

        template class ZeroFillBlock<int8_t>;
        template class ZeroFillBlock<float>;
        template class ZeroFillBlock<complex_t>;
    } // namespace ndsp
} // namespace satdump
