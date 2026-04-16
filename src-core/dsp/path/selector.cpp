#include "selector.h"
#include "dsp/block_helpers.h"
#include <cstdint>
#include <mutex>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        SelectorBlock<T>::SelectorBlock() : Block("selector_" + getShortTypeName<T>() + getShortTypeName<T>(), {{"in1", getTypeSampleType<T>()}}, {{"out1", getTypeSampleType<T>()}})
        {
        }

        template <typename T>
        SelectorBlock<T>::~SelectorBlock()
        {
        }

        template <typename T>
        bool SelectorBlock<T>::work()
        {
            std::scoped_lock l(in_mtx);

            for (int i = 0; i < inputs.size(); i++)
            {
                DSPBuffer iblk;
                if (!inputs[i].fifo->try_dequeue(iblk))
                    continue;

                if (iblk.isTerminator())
                {
                    if (iblk.terminatorShouldPropagate())
                        outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                    inputs[i].fifo->free(iblk);
                    return true;
                }

                if (i == active_id)
                {
                    DSPBuffer oblk = outputs[0].fifo->newBufferSamples(iblk.max_size, sizeof(T));
                    memcpy(oblk.getSamples<T>(), iblk.getSamples<T>(), iblk.size * sizeof(T));
                    oblk.size = iblk.size;
                    outputs[0].fifo->wait_enqueue(oblk);
                }

                inputs[i].fifo->free(iblk);
            }

            return false;
        }

        template class SelectorBlock<complex_t>;
        template class SelectorBlock<float>;
        template class SelectorBlock<int16_t>;
        template class SelectorBlock<int8_t>;
        template class SelectorBlock<uint8_t>;
    } // namespace ndsp
} // namespace satdump