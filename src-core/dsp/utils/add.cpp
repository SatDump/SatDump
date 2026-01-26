#include "add.h"
#include <complex.h>
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        AddBlock<T>::AddBlock()
            : BlockSimpleMulti<T, T, 2, 1>(                                             //
                  "add_" + getShortTypeName<T>(),                                       //
                  {{"in 1", getTypeSampleType<T>()}, {"in 2", getTypeSampleType<T>()}}, //
                  {{"out", getTypeSampleType<T>()}})
        {
        }

        template <typename T>
        AddBlock<T>::~AddBlock()
        {
        }

        template <typename T>
        void AddBlock<T>::process(T **input, uint32_t *nsamples, T **output, uint32_t *nsamples_out)
        {
            if (nsamples[0] != nsamples[1])
            {
                nsamples_out[0] = 0;
                return;
            }

            for (uint32_t i = 0; i < nsamples[0]; i++)
            {
                output[0][i] = input[0][i] + input[1][i];
            }

            nsamples_out[0] = nsamples[0];
        }
        template class AddBlock<float>;
        template class AddBlock<complex_t>;
    } // namespace ndsp
} // namespace satdump
