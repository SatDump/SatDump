#include "switch.h"
#include "dsp/block_helpers.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        SwitchBlock<T>::SwitchBlock() : Block("switch_" + getShortTypeName<T>() + getShortTypeName<T>(), {{"in", getTypeSampleType<T>()}}, {{"out1", getTypeSampleType<T>()}})
        {
        }

        template <typename T>
        SwitchBlock<T>::~SwitchBlock()
        {
        }

        template <typename T>
        bool SwitchBlock<T>::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                if (iblk.terminatorShouldPropagate())
                {
                    out_mtx.lock();
                    for (auto &o : outputs)
                        o.fifo->wait_enqueue(o.fifo->newBufferTerminator());
                    out_mtx.unlock();
                }
                inputs[0].fifo->free(iblk);
                return true;
            }

            out_mtx.lock();
            if (active_id < outputs.size())
            {
                auto &o = outputs[active_id];
                DSPBuffer oblk = o.fifo->newBufferSamples(iblk.max_size, sizeof(T));
                memcpy(oblk.getSamples<T>(), iblk.getSamples<T>(), iblk.size * sizeof(T));
                oblk.size = iblk.size;
                o.fifo->wait_enqueue(oblk);
            }
            out_mtx.unlock();

            inputs[0].fifo->free(iblk);

            return false;
        }

        template class SwitchBlock<complex_t>;
        template class SwitchBlock<float>;
        template class SwitchBlock<int16_t>;
        template class SwitchBlock<int8_t>;
        template class SwitchBlock<uint8_t>;
    } // namespace ndsp
} // namespace satdump