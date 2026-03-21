#include "fft.h"
#include "common/dsp/buffer.h"
#include <fftw3.h>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        FFTFilterBlock<T>::FFTFilterBlock()
            : Block(std::is_same_v<T, complex_t> ? "fft_filter_cc" : "fft_filter_ff", {{"in", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}},
                    {{"out", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}})
        {
        }

        template <typename T>
        FFTFilterBlock<T>::~FFTFilterBlock()
        {
            if (ffttaps_buffer != nullptr)
                volk_free(ffttaps_buffer);
            if (tail != nullptr)
                volk_free(tail);

            if (fft_fwd_in != nullptr)
                fftwf_free(fft_fwd_in);
            if (fft_fwd_out != nullptr)
                fftwf_free(fft_fwd_out);
            if (fft_inv_in != nullptr)
                fftwf_free(fft_inv_in);
            if (fft_inv_out != nullptr)
                fftwf_free(fft_inv_out);
        }

        template <typename T>
        void FFTFilterBlock<T>::init()
        {
            d_ntaps = p_taps.size();
            d_fftsize = (int)(2 * pow(2.0, ceil(log(double(d_ntaps)) / log(2.0)))) * 100;
            d_nsamples = d_fftsize - d_ntaps + 1;

            //  printf("TAPS %d FFT %d SAMP %d\n", d_ntaps, d_fftsize, d_nsamples);

            if (fft_fwd_in != nullptr)
                fftwf_free(fft_fwd_in);
            if (fft_fwd_out != nullptr)
                fftwf_free(fft_fwd_out);
            if (fft_inv_in != nullptr)
                fftwf_free(fft_inv_in);
            if (fft_inv_out != nullptr)
                fftwf_free(fft_inv_out);

            fft_fwd_in = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * d_fftsize);
            fft_fwd_out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * d_fftsize);
            fft_inv_in = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * d_fftsize);
            fft_inv_out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * d_fftsize);

            fft_fwd = fftwf_plan_dft_1d(d_fftsize, fft_fwd_in, fft_fwd_out, FFTW_FORWARD, FFTW_ESTIMATE);
            fft_inv = fftwf_plan_dft_1d(d_fftsize, fft_inv_in, fft_inv_out, FFTW_BACKWARD, FFTW_ESTIMATE);

            // Init buffer
            buffer.resize(8192);
            buffer_size = buffer.size();
            in_buffer = 0;

            // Init taps
            float scale = 1.0 / d_fftsize;
            for (int i = 0; i < d_ntaps; i++)
                ((complex_t *)fft_fwd_in)[i] = complex_t(p_taps[i] * scale, 0.0f);
            for (int i = d_ntaps; i < d_fftsize; i++)
                ((complex_t *)fft_fwd_in)[i] = complex_t(0.0f, 0.0f);

            fftwf_execute(fft_fwd);

            ffttaps_buffer = dsp::create_volk_buffer<T>(d_fftsize);
            for (int i = 0; i < d_fftsize; i++)
                ffttaps_buffer[i] = ((complex_t *)fft_fwd_out)[i];

            tail = dsp::create_volk_buffer<T>(d_ntaps - 1);
        }

        template <typename T>
        bool FFTFilterBlock<T>::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                if (iblk.terminatorShouldPropagate())
                    outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                inputs[0].fifo->free(iblk);
                return true;
            }

            if (needs_reinit)
            {
                needs_reinit = false;
                init();
            }

            int nsamples = iblk.size;

            // Automatically grow buffer
            if (buffer_size < in_buffer + nsamples)
            {
                buffer.resize((buffer_size + nsamples) * 2);
                buffer_size = buffer.size();
            }

            memcpy(&buffer[in_buffer], iblk.getSamples<T>(), nsamples * sizeof(T));
            in_buffer += nsamples;

            int consumed = 0;

            if constexpr (std::is_same_v<T, float>) {}
            if constexpr (std::is_same_v<T, complex_t>)
            {
                for (int i = 0; (i + d_nsamples) < in_buffer; i += d_nsamples)
                {
                    DSPBuffer oblk = outputs[0].fifo->newBufferSamples(d_nsamples, sizeof(T));
                    T *obuf = oblk.getSamples<T>();

                    memcpy(fft_fwd_in, &buffer[i], d_nsamples * sizeof(complex_t));
                    consumed += d_nsamples;

                    for (int j = d_nsamples; j < d_fftsize; j++)
                        ((complex_t *)fft_fwd_in)[j] = complex_t(0.0f, 0.0f);

                    fftwf_execute(fft_fwd);
                    volk_32fc_x2_multiply_32fc_a((lv_32fc_t *)fft_inv_in, (lv_32fc_t *)fft_fwd_out, (lv_32fc_t *)ffttaps_buffer, d_fftsize);
                    fftwf_execute(fft_inv);

                    for (int j = 0; j < (d_ntaps - 1); j++)
                        ((complex_t *)fft_inv_out)[j] += tail[j];

                    memcpy(&obuf[0], (complex_t *)fft_inv_out, d_nsamples * sizeof(complex_t));
                    memcpy(&tail[0], (complex_t *)fft_inv_out + d_nsamples, (d_ntaps - 1) * sizeof(complex_t));

                    oblk.size = d_nsamples;
                    outputs[0].fifo->wait_enqueue(oblk);
                }
            }

            memmove(&buffer[0], &buffer[consumed], (in_buffer - consumed) * sizeof(T));
            in_buffer -= consumed;

            inputs[0].fifo->free(iblk);

            return false;
        }

        template class FFTFilterBlock<complex_t>;
        // template class FFTFilterBlock<float>;
    } // namespace ndsp
} // namespace satdump