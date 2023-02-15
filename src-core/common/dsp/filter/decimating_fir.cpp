#include "decimating_fir.h"
#include <cstring>
#include <volk/volk.h>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

namespace dsp
{
    template <typename T>
    DecimatingFIRBlock<T>::DecimatingFIRBlock(std::shared_ptr<dsp::stream<T>> input, std::vector<float> taps, int decimation) : Block<T, T>(input), d_decimation(decimation)
    {
        // Get alignement parameters
        align = volk_get_alignment();
        aligned_tap_count = std::max<int>(1, align / sizeof(T));

        // Set taps
        ntaps = taps.size();

        // Init taps
        this->taps = (float **)volk_malloc(aligned_tap_count * sizeof(float *), align);
        for (int i = 0; i < aligned_tap_count; i++)
        {
            this->taps[i] = (float *)volk_malloc((ntaps + aligned_tap_count - 1) * sizeof(float), align);
            for (int y = 0; y < ntaps + aligned_tap_count - 1; y++)
                this->taps[i][y] = 0;
            for (int j = 0; j < ntaps; j++)
                this->taps[i][i + j] = taps[(ntaps - 1) - j]; // Reverse taps
        }

        // Init history buffer
        buffer = create_volk_buffer<T>(2 * STREAM_BUFFER_SIZE);
    }

    template <typename T>
    DecimatingFIRBlock<T>::~DecimatingFIRBlock()
    {
        for (int i = 0; i < aligned_tap_count; i++)
            volk_free(taps[i]);
        volk_free(taps);
        volk_free(buffer);
    }

    template <typename T>
    int DecimatingFIRBlock<T>::process(T *input, int nsamples, T *output)
    {
        memcpy(&buffer[ntaps], input, nsamples * sizeof(T));

        outc = 0;
        if constexpr (std::is_same_v<T, float>)
        {
            for (; inc < nsamples; inc += d_decimation)
            {
                // Doing it this way instead of the normal :
                // volk_32f_x2_dot_prod_32f(&output_stream->writeBuf[i], &buffer[i + 1], taps, ntaps);
                // turns out to enhance performances quite a bit... Initially tested this after noticing
                // inconsistencies between the above simple way and GNU Radio's implementation
                const float *ar = (float *)((size_t)&buffer[inc + 1] & ~(align - 1));
                const unsigned al = &buffer[inc + 1] - ar;
                volk_32f_x2_dot_prod_32f_a(&output[outc++], ar, taps[al], ntaps + al);
            }
        }
        if constexpr (std::is_same_v<T, complex_t>)
        {
            for (; inc < nsamples; inc += d_decimation)
            {
                // Doing it this way instead of the normal :
                // volk_32fc_32f_dot_prod_32fc(&output_stream->writeBuf[i], &buffer[i + 1], taps, ntaps);
                // turns out to enhance performances quite a bit... Initially tested this after noticing
                // inconsistencies between the above simple way and GNU Radio's implementation
                const complex_t *ar = (complex_t *)((size_t)&buffer[inc + 1] & ~(align - 1));
                const unsigned al = &buffer[inc + 1] - ar;
                volk_32fc_32f_dot_prod_32fc_a((lv_32fc_t *)&output[outc++], (lv_32fc_t *)ar, taps[al], ntaps + al);
            }
        }

        inc -= nsamples;

        memmove(&buffer[0], &buffer[nsamples], ntaps * sizeof(T));
        return outc;
    }

    template <typename T>
    void DecimatingFIRBlock<T>::work()
    {
        int nsamples = Block<T, T>::input_stream->read();
        if (nsamples <= 0)
        {
            Block<T, T>::input_stream->flush();
            return;
        }

        int outn = process(Block<T, T>::input_stream->readBuf, nsamples, Block<T, T>::output_stream->writeBuf);

        Block<T, T>::input_stream->flush();
        Block<T, T>::output_stream->swap(outn);
    }

    template class DecimatingFIRBlock<complex_t>;
    template class DecimatingFIRBlock<float>;
}