#include "fft_pan.h"
#include "logger.h"
#include "window.h"

namespace dsp
{
    FFTPanBlock::FFTPanBlock(std::shared_ptr<dsp::stream<complex_t>> input)
        : Block(input)
    {
        // fft_main_buffer = new complex_t[STREAM_BUFFER_SIZE * 4];
    }

    FFTPanBlock::~FFTPanBlock()
    {
    }

    void FFTPanBlock::set_fft_settings(int size)
    {
        fft_mutex.lock();

        fft_size = size;

        if (fft_output_buffer != nullptr)
            destroy_fft();

        // Init taps, rectangular window
        fft_taps.resize(fft_size);
        for (int i = 0; i < fft_size; i++)
            fft_taps[i] = window::nuttall(i, fft_size) * ((i % 2) ? 1.0f : -1.0f);

        // Init FFTW
        fftw_in = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * fft_size);
        fftw_out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * fft_size);
        fftw_plan = fftwf_plan_dft_1d(fft_size, fftw_in, fftw_out, FFTW_FORWARD, FFTW_ESTIMATE);
        fft_mutex.unlock();

        memset(fftw_in, 0, sizeof(fftwf_complex) * fft_size);
        memset(fftw_out, 0, sizeof(fftwf_complex) * fft_size);

        // Output buffer
        fft_output_buffer = new float[fft_size];
    }

    void FFTPanBlock::destroy_fft()
    {
        fftwf_free(fftw_in);
        fftwf_free(fftw_out);
        fftwf_destroy_plan(fftw_plan);
        delete[] fft_output_buffer;
    }

    void FFTPanBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

#if 1
        fft_mutex.lock();

        complex_t *buffer_ptr = input_stream->readBuf;

        volk_32fc_32f_multiply_32fc((lv_32fc_t *)fftw_in, (lv_32fc_t *)buffer_ptr, fft_taps.data(), fft_size);

        input_stream->flush();

        fftwf_execute(fftw_plan);

        volk_32fc_s32f_power_spectrum_32f(fft_output_buffer, (lv_32fc_t *)fftw_out, 1, fft_size);

        for (int i = 0; i < fft_size; i++)
            output_stream->writeBuf[i] = output_stream->writeBuf[i] * (1.0 - avg_rate) + fft_output_buffer[i] * avg_rate;
        // output_stream->swap(fft_size);

        fft_mutex.unlock();
#else
        if (in_main_buffer + nsamples < STREAM_BUFFER_SIZE * 4)
        {
            memcpy(&fft_main_buffer[in_main_buffer], input_stream->readBuf, nsamples * sizeof(complex_t));
            in_main_buffer += nsamples;
        }
        input_stream->flush();

        if (in_main_buffer > fft_size)
        {
            fft_mutex.lock();
            int position_ptr = 0;
            while (in_main_buffer - position_ptr > fft_size)
            {
                complex_t *buffer_ptr = &fft_main_buffer[position_ptr];

                volk_32fc_32f_multiply_32fc((lv_32fc_t *)fftw_in, (lv_32fc_t *)buffer_ptr, fft_taps, fft_size);

                fftwf_execute(fftw_plan);

                volk_32fc_s32f_power_spectrum_32f(fft_output_buffer, (lv_32fc_t *)fftw_out, 1, fft_size);

                for (int i = 0; i < fft_size; i++)
                    output_stream->writeBuf[i] = output_stream->writeBuf[i] * (1.0 - avg_rate) + fft_output_buffer[i] * avg_rate;
                // output_stream->swap(fft_size);

                position_ptr += fft_size;
            }
            fft_mutex.unlock();
            in_main_buffer = 0;
        }
#endif
    }
}