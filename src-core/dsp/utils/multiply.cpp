#include "multiply.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        MultiplyBlock<T>::MultiplyBlock()
            : BlockSimpleMulti<T, T, 2, 1>(                                             //
                  "multiply_" + getShortTypeName<T>(),                                  //
                  {{"in 1", getTypeSampleType<T>()}, {"in 2", getTypeSampleType<T>()}}, //
                  {{"out", getTypeSampleType<T>()}})
        {
        }

        template <typename T>
        MultiplyBlock<T>::~MultiplyBlock()
        {
        }

        template <typename T>
        void MultiplyBlock<T>::process(T **input, uint32_t *nsamples, T **output, uint32_t *nsamples_out)
        {

            if (nsamples[0] != nsamples[1])
            {
                nsamples_out[0] = 0;
                return;
            }

            for (uint32_t i = 0; i < nsamples[0]; i++)
            {
                output[0][i] = input[0][i] * input[1][i];
            }

            nsamples_out[0] = nsamples[0];
        }
        template class MultiplyBlock<float>;
        template class MultiplyBlock<complex_t>;
    } // namespace ndsp
} // namespace satdump
