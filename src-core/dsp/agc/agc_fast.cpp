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

            // Multiply by gain and compute mags
            if constexpr (std::is_same_v<T, complex_t>)
            {
                volk_32f_s32f_multiply_32f((float *)out, (float *)in, gain, nsamples * 2);
                volk_32fc_magnitude_32f(mag_buf.data(), (const lv_32fc_t *)out, nsamples);
            }
            else if constexpr (std::is_same_v<T, float>)
            {
                volk_32f_s32f_multiply_32f(out, in, gain, nsamples);
                for (uint32_t i = 0; i < nsamples; i++)
                    mag_buf[i] = fabsf(out[i]);
            }

            // Update gain. Copying to consts seems faster here too...
            const float lrate = rate;
            const float lref = reference;
            float lgain = gain;

            for (uint32_t i = 0; i < nsamples; i++)
                lgain += lrate * (lref - mag_buf[i]);

            // Clamp gain
            gain = fminf(lgain, max_gain);

            return nsamples;
        }

        template class AGCFastBlock<complex_t>;
        template class AGCFastBlock<float>;
    } // namespace ndsp
} // namespace satdump