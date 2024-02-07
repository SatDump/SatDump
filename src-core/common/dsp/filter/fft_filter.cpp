#include "fft_filter.h"
#include <cstring>
#include <volk/volk.h>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

namespace dsp
{
    template <typename T>
    FFTFilterBlock<T>::FFTFilterBlock(std::shared_ptr<dsp::stream<T>> input, std::vector<float> taps) : Block<T, T>(input)
    {
        d_ntaps = taps.size();
        d_fftsize = (int)(2 * pow(2.0, ceil(log(double(d_ntaps)) / log(2.0)))) * 100;
        d_nsamples = d_fftsize - d_ntaps + 1;

        printf("TAPS %d FFT %d SAMP %d\n", d_ntaps, d_fftsize, d_nsamples);

        fft_fwd_in = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * d_fftsize);
        fft_fwd_out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * d_fftsize);
        fft_inv_in = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * d_fftsize);
        fft_inv_out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * d_fftsize);

        fft_fwd = fftwf_plan_dft_1d(d_fftsize, fft_fwd_in, fft_fwd_out, FFTW_FORWARD, FFTW_ESTIMATE);
        fft_inv = fftwf_plan_dft_1d(d_fftsize, fft_inv_in, fft_inv_out, FFTW_BACKWARD, FFTW_ESTIMATE);

        // Init buffer
        buffer = create_volk_buffer<T>(2 * STREAM_BUFFER_SIZE);

        // Init taps
        float scale = 1.0 / d_fftsize;
        for (int i = 0; i < d_ntaps; i++)
            ((complex_t *)fft_fwd_in)[i] = complex_t(taps[i] * scale, 0.0f);
        for (int i = d_ntaps; i < d_fftsize; i++)
            ((complex_t *)fft_fwd_in)[i] = complex_t(0.0f, 0.0f);
        fftwf_execute(fft_fwd);
        ffttaps_buffer = create_volk_buffer<T>(d_fftsize);
        for (int i = 0; i < d_fftsize; i++)
            ffttaps_buffer[i] = ((complex_t *)fft_fwd_out)[i];

        tail = create_volk_buffer<T>(d_ntaps - 1);
    }

    template <typename T>
    FFTFilterBlock<T>::~FFTFilterBlock()
    {
        volk_free(buffer);
        volk_free(ffttaps_buffer);
        volk_free(tail);
    }

    template <typename T>
    void FFTFilterBlock<T>::work()
    {
        int nsamples = Block<T, T>::input_stream->read();
        if (nsamples <= 0)
        {
            Block<T, T>::input_stream->flush();
            return;
        }

        memcpy(&buffer[in_buffer], Block<T, T>::input_stream->readBuf, nsamples * sizeof(T));
        in_buffer += nsamples;

        int consumed = 0;

        if constexpr (std::is_same_v<T, float>)
        {
        }
        if constexpr (std::is_same_v<T, complex_t>)
        {
            for (int i = 0; (i + d_nsamples) < in_buffer; i += d_nsamples)
            {
                memcpy(fft_fwd_in, &buffer[i], d_nsamples * sizeof(complex_t));
                consumed += d_nsamples;

                for (int j = d_nsamples; j < d_fftsize; j++)
                    ((complex_t *)fft_fwd_in)[j] = complex_t(0.0f, 0.0f);

                fftwf_execute(fft_fwd);
                volk_32fc_x2_multiply_32fc_a((lv_32fc_t *)fft_inv_in, (lv_32fc_t *)fft_fwd_out, (lv_32fc_t *)ffttaps_buffer, d_fftsize);
                fftwf_execute(fft_inv);

                for (int j = 0; j < (d_ntaps - 1); j++)
                    ((complex_t *)fft_inv_out)[j] += tail[j];

                memcpy(&Block<T, T>::output_stream->writeBuf[i], fft_inv_out, d_nsamples * sizeof(complex_t));
                memcpy(&tail[0], fft_inv_out + d_nsamples, (d_ntaps - 1) * sizeof(complex_t));
            }
        }

        Block<T, T>::input_stream->flush();
        Block<T, T>::output_stream->swap(consumed);

        memmove(&buffer[0], &buffer[consumed], (in_buffer - consumed) * sizeof(T));
        in_buffer -= consumed;
    }

    template class FFTFilterBlock<complex_t>;
    // template class FFTFilterBlock<float>;
}