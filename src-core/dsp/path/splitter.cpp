#include "splitter.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        SplitterBlock<T>::SplitterBlock()
            : Block(std::is_same_v<T, complex_t> ? "splitter_cc" : "splitter_ff", {{"in", getTypeSampleType<T>()}},
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
                    // Snapshot outputs under lock to avoid deadlock:
                    // wait_enqueue() can block if FIFO is full, and del_output()
                    // also tries to acquire vfos_mtx → would deadlock.
                    std::vector<BlockIO> outputs_snapshot;
                    {
                        std::lock_guard<std::mutex> lock(vfos_mtx);
                        outputs_snapshot = outputs;
                    }
                    // Enqueue terminators WITHOUT holding vfos_mtx
                    for (auto &o : outputs_snapshot)
                    {
                        IOInfo *i = ((IOInfo *)o.blkdata.get());
                        if (i->forward_terminator)
                            o.fifo->wait_enqueue(o.fifo->newBufferTerminator());
                    }
                }
                inputs[0].fifo->free(iblk);
                return true;
            }

            // Snapshot outputs under lock, then release before blocking enqueue.
            // This prevents deadlock when del_output()/add_output() is called
            // while a FIFO output is full and wait_enqueue() would block.
            std::vector<BlockIO> outputs_snapshot;
            {
                std::lock_guard<std::mutex> lock(vfos_mtx);
                outputs_snapshot = outputs;
            }

            // Enqueue data WITHOUT holding vfos_mtx
            for (auto &o : outputs_snapshot)
            {
                DSPBuffer oblk = o.fifo->newBufferSamples(iblk.max_size, sizeof(T));
                memcpy(oblk.getSamples<T>(), iblk.getSamples<T>(), iblk.size * sizeof(T));
                oblk.size = iblk.size;
                o.fifo->wait_enqueue(oblk);
            }

            inputs[0].fifo->free(iblk);

            return false;
        }

        template class SplitterBlock<complex_t>;
        template class SplitterBlock<float>;
        template class SplitterBlock<int16_t>;
        template class SplitterBlock<int8_t>;
        template class SplitterBlock<uint8_t>;
    } // namespace ndsp
} // namespace satdump