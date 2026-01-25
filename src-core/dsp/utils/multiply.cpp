#include "multiply.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        MultiplyBlock<T>::MultiplyBlock()
            : BlockSimpleMulti<T, T, 2, 1>(                               //
                  "add_" + getShortTypeName<T>() + getShortTypeName<T>(), //
                  {{"in", getTypeSampleType<T>()}},                       //
                  {{"out", getTypeSampleType<T>()}})
        {
        }

        template <typename T>
        MultiplyBlock<T>::~MultiplyBlock()
        {
        }

        template <typename T>
        void MultiplyBlock<T>::process(T *input_a, T *input_b, uint32_t nsamples, T *output, uint32_t nsamples_out)
        {
            // nsamples_out = nsamples;
            if constexpr (std::is_same_v<T, float>)
            {
                for (uint32_t i = 0; i < nsamples; i++)
                {
                    volk_32f_x2_multiply_32f((float *)output, (const float *)input_a, (const float *)input_b, nsamples);
                }
            }

            if constexpr (std::is_same_v<T, complex_t>)
            {
                for (uint32_t i = 0; i < nsamples; i++)
                {
                    volk_32fc_x2_multiply_32fc((lv_32fc_t *)output, (const lv_32fc_t *)input_a, (const lv_32fc_t *)input_b, nsamples);
                }
            }
        }
    } // namespace ndsp
} // namespace satdump
