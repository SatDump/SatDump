#include "repeat.h"
#include "dsp/block_helpers.h"
#include <complex.h>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        RepeatBlock<T>::RepeatBlock() : Block("repeat_" + getShortTypeName<T>() + getShortTypeName<T>(), {{"in", getTypeSampleType<T>()}}, {{"out", getTypeSampleType<T>()}})
        {
        }

        template <typename T>
        RepeatBlock<T>::~RepeatBlock()
        {
        }

        template <typename T>
        bool RepeatBlock<T>::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                if (iblk.terminatorShouldPropagate())
                    outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                inputs[0].fifo->free(iblk);
                return true;
            }

            const int intpo = interpolation;

            DSPBuffer oblk = outputs[0].fifo->newBufferSamples(intpo * iblk.size, sizeof(T));
            T *ibuf = iblk.getSamples<T>();
            T *obuf = oblk.getSamples<T>();

            for (int i = 0; i < iblk.size; i++)
                for (int ii = 0; ii < intpo; ii++)
                    obuf[i * intpo + ii] = ibuf[i];

            oblk.size = iblk.size * intpo;
            outputs[0].fifo->wait_enqueue(oblk);
            inputs[0].fifo->free(iblk);

            return false;
        }

        template class RepeatBlock<int8_t>;
        template class RepeatBlock<float>;
        template class RepeatBlock<complex_t>;
    } // namespace ndsp
} // namespace satdump
