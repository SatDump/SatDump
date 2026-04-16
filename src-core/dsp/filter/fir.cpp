#include "fir.h"
#include "dsp/block_helpers.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T, typename TT>
        FIRBlock<T, TT>::FIRBlock(std::string id)
            : BlockSimple<T, T>(id + "fir" + (std::is_same_v<TT, complex_t> ? "c" : "") + "_" + getShortTypeName<T>() + getShortTypeName<T>(), {{"in", getTypeSampleType<T>()}},
                                {{"out", getTypeSampleType<T>()}})
        {
        }

        template <typename T, typename TT>
        FIRBlock<T, TT>::~FIRBlock()
        {
            if (taps != nullptr)
            {
                for (int i = 0; i < aligned_tap_count; i++)
                    volk_free(taps[i]);
                volk_free(taps);
            }
        }

        template <typename T, typename TT>
        void FIRBlock<T, TT>::init()
        {
            // Free if needed
            if (taps != nullptr)
            {
                for (int i = 0; i < aligned_tap_count; i++)
                    volk_free(taps[i]);
                volk_free(taps);
            }

            // Init buffer
            buffer.resize(8192);
            buffer_size = buffer.size();
            in_buffer = 0;

            // Get alignement parameters
            align = volk_get_alignment();
            aligned_tap_count = std::max<int>(1, align / sizeof(T));

            // Set taps
            ntaps = p_taps.size();

            // Init taps
            this->taps = (TT **)volk_malloc(aligned_tap_count * sizeof(TT *), align);
            for (int i = 0; i < aligned_tap_count; i++)
            {
                this->taps[i] = (TT *)volk_malloc((ntaps + aligned_tap_count - 1) * sizeof(TT), align);
                for (int y = 0; y < ntaps + aligned_tap_count - 1; y++)
                    this->taps[i][y] = 0;
                for (int j = 0; j < ntaps; j++)
                    this->taps[i][i + j] = p_taps[(ntaps - 1) - j]; // Reverse taps
            }
        }

        template <typename T, typename TT>
        uint32_t FIRBlock<T, TT>::process(T *input, uint32_t nsamples, T *output)
        {
            if (needs_reinit)
            {
                needs_reinit = false;
                init();
            }

            // Automatically grow buffer
            if (buffer_size < in_buffer + nsamples)
            {
                buffer.resize((buffer_size + nsamples) * 2);
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
                if constexpr (std::is_same_v<TT, float>)
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
                if constexpr (std::is_same_v<TT, complex_t>)
                {
                    for (int i = 0; i < to_use; i++)
                    {
                        // Doing it this way instead of the normal :
                        // volk_32fc_32f_dot_prod_32fc(&output_stream->writeBuf[i], &buffer[i + 1], taps, ntaps);
                        // turns out to enhance performances quite a bit... Initially tested this after noticing
                        // inconsistencies between the above simple way and GNU Radio's implementation
                        const complex_t *ar = (complex_t *)((size_t)&buffer[i + 1] & ~(align - 1));
                        const unsigned al = &buffer[i + 1] - ar;
                        volk_32fc_x2_dot_prod_32fc_a((lv_32fc_t *)&output[i], (lv_32fc_t *)ar, (lv_32fc_t *)taps[al], ntaps + al);
                    }
                }
            }

            memmove(&buffer[0], &buffer[to_use], (in_buffer - to_use) * sizeof(T));
            in_buffer -= to_use;

            return to_use;
        }

        template class FIRBlock<complex_t, complex_t>;
        template class FIRBlock<complex_t, float>;
        template class FIRBlock<float, float>;
    } // namespace ndsp
} // namespace satdump