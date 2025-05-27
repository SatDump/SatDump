#include "agc.h"
#include "common/dsp/complex.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        AGCBlock<T>::AGCBlock()
            : BlockSimple<T, T>(std::is_same_v<T, complex_t> ? "agc_cc" : "agc_ff", {{"in", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}},
                                {{"out", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}})
        {
        }

        template <typename T>
        AGCBlock<T>::~AGCBlock()
        {
        }

        template <typename T>
        uint32_t AGCBlock<T>::process(T *in, uint32_t nsamples, T *out)
        {
            for (uint32_t i = 0; i < nsamples; i++)
            {
                T output = in[i] * gain;

                if constexpr (std::is_same_v<T, float>)
                    gain += rate * (reference - fabsf(output));
                if constexpr (std::is_same_v<T, complex_t>)
                    gain += rate * (reference - sqrt(output.real * output.real + output.imag * output.imag));

                if (max_gain > 0.0 && gain > max_gain)
                    gain = max_gain;

                out[i] = output;
            }

            return nsamples;
        }

        template class AGCBlock<complex_t>;
        template class AGCBlock<float>;
    } // namespace ndsp
} // namespace satdump