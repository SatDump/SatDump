#include "agc_fast.h"
#include "common/dsp/complex.h"
#include <volk/volk.h>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        AGCFastBlock<T>::AGCFastBlock()
            : BlockSimple<T, T>(std::is_same_v<T, complex_t> ? "agc_fast_cc" : "agc_fast_ff", {{"in", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}},
                                {{"out", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}})
        {
        }

        template <typename T>
        AGCFastBlock<T>::~AGCFastBlock()
        {
        }

        template <typename T>
        uint32_t AGCFastBlock<T>::process(T *in, uint32_t nsamples, T *out)
        {
            // Automatically resize
            if (nsamples > mag_buf_size)
            {
                mag_buf.resize(nsamples);
                mag_buf_size = mag_buf.size();
            }

            // Compute mags
            if constexpr (std::is_same_v<T, complex_t>)
            {
                volk_32fc_magnitude_32f(mag_buf.data(), (const lv_32fc_t *)in, nsamples);
            }
            else if constexpr (std::is_same_v<T, float>)
            {
                for (uint32_t i = 0; i < nsamples; i++)
                    mag_buf[i] = fabsf(out[i]);
            }

            // Calculate with gain
            for (uint32_t i = 0; i < nsamples; i++)
            {
                out[i] = in[i] * gain;

                if constexpr (std::is_same_v<T, float>)
                    gain += rate * (reference - (mag_buf[i] * gain));
                if constexpr (std::is_same_v<T, complex_t>)
                    gain += rate * (reference - (mag_buf[i] * gain));

                if (max_gain > 0.0 && gain > max_gain)
                    gain = max_gain;
            }

            return nsamples;
        }

        template class AGCFastBlock<complex_t>;
        template class AGCFastBlock<float>;
    } // namespace ndsp
} // namespace satdump