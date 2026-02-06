#include "blk_agc.h"
#include "common/dsp/complex.h"
#include <cstdint>
#include <volk/volk.h>
#include <volk/volk_complex.h>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        BlkAGCBlock<T>::BlkAGCBlock()
            : BlockSimple<T, T>(std::is_same_v<T, complex_t> ? "blk_agc_cc" : "blk_agc_ff", {{"in", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}},
                                {{"out", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}})
        {
        }

        template <typename T>
        BlkAGCBlock<T>::~BlkAGCBlock()
        {
        }

        template <typename T>
        uint32_t BlkAGCBlock<T>::process(T *in, uint32_t nsamples, T *out)
        {
            uint32_t max_pos = 0;

            if constexpr (std::is_same_v<T, float>)
            {
                volk_32f_s32f_multiply_32f((float *)out, (float *)in, gain, nsamples * 2);

#if 1
                volk_32f_index_max_32u(&max_pos, (float *)out, nsamples);
                T output = out[max_pos];
#else
                T output = 0;
                for (uint32_t i = 0; i < nsamples; i++)
                    output += out[i];
                output /= T(nsamples);
#endif

                gain += rate * (reference - fabsf(output)) * nsamples;
            }
            if constexpr (std::is_same_v<T, complex_t>)
            {
                volk_32f_s32f_multiply_32f((float *)out, (float *)in, gain, nsamples * 2);

#if 1
                volk_32fc_index_max_32u(&max_pos, (lv_32fc_t *)out, nsamples);
                T output = out[max_pos];
#else
                T output = 0;
                for (uint32_t i = 0; i < nsamples; i++)
                    output += out[i];
                output = output / float(nsamples);
#endif

                gain += rate * (reference - sqrt(output.real * output.real + output.imag * output.imag)) * nsamples;
            }

            if (max_gain > 0.0 && gain > max_gain)
                gain = max_gain;

            return nsamples;
        }

        template class BlkAGCBlock<complex_t>;
        template class BlkAGCBlock<float>;
    } // namespace ndsp
} // namespace satdump