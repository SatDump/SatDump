#include "reshape_buffer.h"
#include "dsp/block_helpers.h"
#include <complex.h>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        ReshapeBufferBlock<T>::ReshapeBufferBlock() : Block("reshape_buffer_" + getShortTypeName<T>() + getShortTypeName<T>(), {{"in", getTypeSampleType<T>()}}, {{"out", getTypeSampleType<T>()}})
        {
        }

        template <typename T>
        ReshapeBufferBlock<T>::~ReshapeBufferBlock()
        {
        }

        template <typename T>
        bool ReshapeBufferBlock<T>::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                if (iblk.terminatorShouldPropagate())
                    outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                inputs[0].fifo->free(iblk);
                return true;
            }

            // Put into buffer
            buffer.insert(buffer.end(), iblk.getSamples<T>(), iblk.getSamples<T>() + iblk.size);

            // While we got enough elements in the buffer, output them
            const int bufsize = buffer_size;
            while (buffer.size() >= bufsize)
            {
                DSPBuffer oblk = outputs[0].fifo->newBufferSamples(bufsize, sizeof(T));
                memcpy(oblk.getSamples<T>(), buffer.data(), bufsize * sizeof(T));
                buffer.erase(buffer.begin(), buffer.begin() + bufsize);
                oblk.size = bufsize;
                outputs[0].fifo->wait_enqueue(oblk);
            }

            inputs[0].fifo->free(iblk);

            return false;
        }

        template class ReshapeBufferBlock<int8_t>;
        template class ReshapeBufferBlock<float>;
        template class ReshapeBufferBlock<complex_t>;
    } // namespace ndsp
} // namespace satdump
