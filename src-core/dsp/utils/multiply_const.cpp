#include "multiply_const.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        MultiplyConstBlock<T>::MultiplyConstBlock()
            : BlockSimple<T, T>(                            //
                  "multiply_const" + getShortTypeName<T>(), //
                  {{"in", getTypeSampleType<T>()}},         //
                  {{"out", getTypeSampleType<T>()}})
        {
        }

        template <typename T>
        MultiplyConstBlock<T>::~MultiplyConstBlock()
        {
        }

        template <typename T>
        uint32_t MultiplyConstBlock<T>::process(T *input, uint32_t nsamples, T *output)
        {

            for (uint32_t i = 0; i < nsamples; i++)
                output[i] = input[i] * mult_const;

            return nsamples;
        }

        template class MultiplyConstBlock<float>;
        template class MultiplyConstBlock<complex_t>;
    } // namespace ndsp
} // namespace satdump
