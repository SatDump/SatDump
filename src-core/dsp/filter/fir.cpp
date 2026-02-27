#include "fir.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        FIRBlock<T>::FIRBlock()
            : BlockSimple<T, T>(std::is_same_v<T, complex_t> ? "fir_cc" : "fir_ff", {{"in", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}},
                                {{"out", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}})
        {
        }

        template <typename T>
        FIRBlock<T>::~FIRBlock()
        {
            if (taps != nullptr)
            {
                for (int i = 0; i < aligned_tap_count; i++)
                    volk_free(taps[i]);
                volk_free(taps);
            }
        }

        template <typename T>
        uint32_t FIRBlock<T>::process(T *input, uint32_t nsamples, T *output)
        {
            if (needs_reinit)
            {
                needs_reinit = false;
                init();
            }

            // Automatically grow buffer
            if (buffer_size < in_buffer + nsamples)
            {
                buffer.resize(buffer_size * 2);
                buffer_size = buffer.size();
            }

            memcpy(&buffer[in_buffer], input, nsamples * sizeof(T));
            in_buffer += nsamples;

            int to_use = (int)in_buffer - ntaps;
            if (to_use < 0)
                to_use = 0;

            if constexpr (std::is_same_v<T, float>)
            {
                for (int i = 0; i < to_use; i++)
                {
                    // Doing it this way instead of the normal :
                    // volk_32f_x2_dot_prod_32f(&output_stream->writeBuf[i], &buffer[i + 1], taps, ntaps);
                    // turns out to enhance performances quite a bit... Initially tested this after noticing
                    // inconsistencies between the above simple way and GNU Radio's implementation
                    const float *ar = (float *)((size_t)&buffer[i + 1] & ~(align - 1));
                    const unsigned al = &buffer[i + 1] - ar;
                    volk_32f_x2_dot_prod_32f_a(&output[i], ar, taps[al], ntaps + al);
                }
            }
            if constexpr (std::is_same_v<T, complex_t>)
            {
                for (int i = 0; i < to_use; i++)
                {
                    // Doing it this way instead of the normal :
                    // volk_32fc_32f_dot_prod_32fc(&output_stream->writeBuf[i], &buffer[i + 1], taps, ntaps);
                    // turns out to enhance performances quite a bit... Initially tested this after noticing
                    // inconsistencies between the above simple way and GNU Radio's implementation
                    const complex_t *ar = (complex_t *)((size_t)&buffer[i + 1] & ~(align - 1));
                    const unsigned al = &buffer[i + 1] - ar;
                    volk_32fc_32f_dot_prod_32fc_a((lv_32fc_t *)&output[i], (lv_32fc_t *)ar, taps[al], ntaps + al);
                }
            }

            memmove(&buffer[0], &buffer[to_use], (in_buffer - to_use) * sizeof(T));
            in_buffer -= to_use;

            return to_use;
        }

        template class FIRBlock<complex_t>;
        template class FIRBlock<float>;
    } // namespace ndsp
} // namespace satdump