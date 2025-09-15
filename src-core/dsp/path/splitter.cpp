#include "splitter.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        SplitterBlock<T>::SplitterBlock()
            : Block(std::is_same_v<T, complex_t> ? "splitter_cc" : "splitter_ff", {{"in", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}},
                    {/*{"out", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}*/})
        {
        }

        template <typename T>
        SplitterBlock<T>::~SplitterBlock()
        {
        }

        template <typename T>
        bool SplitterBlock<T>::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                if (iblk.terminatorShouldPropagate())
                {
                    vfos_mtx.lock();
                    for (auto &o : outputs)
                    {
                        IOInfo *i = ((IOInfo *)o.blkdata.get());
                        if (i->forward_terminator)
                            o.fifo->wait_enqueue(o.fifo->newBufferTerminator());
                    }
                    vfos_mtx.unlock();
                }
                inputs[0].fifo->free(iblk);
                return true;
            }

            vfos_mtx.lock();
            for (auto &o : outputs)
            {
                IOInfo *i = ((IOInfo *)o.blkdata.get());

                DSPBuffer oblk = o.fifo->template newBufferSamples<T>(iblk.max_size);
                memcpy(oblk.getSamples<T>(), iblk.getSamples<T>(), iblk.size * sizeof(T));
                oblk.size = iblk.size;
                o.fifo->wait_enqueue(oblk);
            }
            vfos_mtx.unlock();

            inputs[0].fifo->free(iblk);

            return false;
        }

        template class SplitterBlock<complex_t>;
        template class SplitterBlock<float>;
    } // namespace ndsp
} // namespace satdump