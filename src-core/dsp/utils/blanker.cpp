#include "blanker.h"
#include "dsp/block_helpers.h"
#include <cstring>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        BlankerBlock<T>::BlankerBlock() : BlockSimple<T, T>("blanker_" + getShortTypeName<T>() + getShortTypeName<T>(), {{"in", getTypeSampleType<T>()}}, {{"out", getTypeSampleType<T>()}})
        {
        }

        template <typename T>
        BlankerBlock<T>::~BlankerBlock()
        {
        }

        template <typename T>
        uint32_t BlankerBlock<T>::process(T *input, uint32_t nsamples, T *output)
        {
            if (blank)
                memset(output, 0, nsamples * sizeof(T));
            else
                memcpy(output, input, nsamples * sizeof(T));

            return nsamples;
        }

        template class BlankerBlock<float>;
        template class BlankerBlock<complex_t>;
    } // namespace ndsp
} // namespace satdump
