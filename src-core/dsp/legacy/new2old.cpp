#include "new2old.h"
#include "dsp/block_helpers.h"
#include <cstring>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        New2OldBlock<T>::New2OldBlock()
            : Block("new2old_" + getShortTypeName<T>(), //
                    {{"out", getTypeSampleType<T>()}}, {})
        {
        }

        template <typename T>
        New2OldBlock<T>::~New2OldBlock()
        {
        }

        template <typename T>
        bool New2OldBlock<T>::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                inputs[0].fifo->free(iblk);
                return true;
            }

            memcpy(old_stream->writeBuf, iblk.getSamples<T>(), iblk.size * sizeof(T));
            old_stream->swap(iblk.size);

            inputs[0].fifo->free(iblk);

            return false;
        }

        template class New2OldBlock<complex_t>;
        template class New2OldBlock<float>;
        template class New2OldBlock<int16_t>;
        template class New2OldBlock<int8_t>;
        template class New2OldBlock<uint8_t>;
    } // namespace ndsp
} // namespace satdump