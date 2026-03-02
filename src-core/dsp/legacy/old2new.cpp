#include "old2new.h"
#include "dsp/block.h"
#include "dsp/block_helpers.h"
#include <cstdint>
#include <cstring>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        Old2NewBlock<T>::Old2NewBlock()
            : Block("old2new_" + getShortTypeName<T>(), {}, //
                    {{"out", getTypeSampleType<T>()}})
        {
        }

        template <typename T>
        Old2NewBlock<T>::~Old2NewBlock()
        {
        }

        template <typename T>
        bool Old2NewBlock<T>::work()
        {
            if (old_stream_exit)
            {
                outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                return true;
            }
            else
            {
                int nsamples = old_stream->read();

                if (nsamples > 0)
                {
                    auto oblk = outputs[0].fifo->newBufferSamples(nsamples, sizeof(T));

                    memcpy(oblk.template getSamples<T>(), old_stream->readBuf, nsamples * sizeof(T));

                    oblk.size = nsamples;
                    outputs[0].fifo->wait_enqueue(oblk);
                }

                old_stream->flush();
                return false;
            }
        }

        template class Old2NewBlock<complex_t>;
        template class Old2NewBlock<float>;
    } // namespace ndsp
} // namespace satdump