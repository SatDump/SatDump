#include "fft_pan.h"
#include "common/dsp/block.h"
#include "common/dsp/window/window.h"
#include "logger.h"

namespace satdump
{
    namespace ndsp
    {
        FFTPanBlock::FFTPanBlock() : Block("fft_pan_cc", {{"in", DSP_SAMPLE_TYPE_CF32}}, {})
        {
            output_fft_buff = new float[dsp::STREAM_BUFFER_SIZE]; // TODOREWORK
        }

        FFTPanBlock::~FFTPanBlock()
        {
            if (fft_output_buffer != nullptr)
                destroy_fft();
            delete[] output_fft_buff;
        }

        void FFTPanBlock::set_fft_settings(int size, uint64_t samplerate, int rate)
        {
            fft_mutex.lock();

            if (rate < 1)
                rate = 1;

            fft_size = size;

            if (fft_output_buffer != nullptr)
                destroy_fft();

            // Compute FFT settings
            rbuffer_rate = (samplerate / rate);
            rbuffer_size = std::min<int>(rbuffer_rate, fft_size);
            rbuffer_skip = rbuffer_rate - rbuffer_size;
            logger->trace("FFT Rate %d, Samplerate %d, Final Size %d, Skip %d", rbuffer_rate, samplerate, rbuffer_size, rbuffer_skip);

            // Init taps, rectangular window
            fft_taps.resize(rbuffer_size);
            for (int i = 0; i < rbuffer_size; i++)
                fft_taps[i] = dsp::window::nuttall(i, rbuffer_size - 1) * ((i % 2) ? 1.0f : -1.0f);

            // Init FFTW
            fftw_in = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * fft_size);
            fftw_out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * fft_size);
            fftw_plan = fftwf_plan_dft_1d(fft_size, fftw_in, fftw_out, FFTW_FORWARD, FFTW_ESTIMATE);

            memset(fftw_in, 0, sizeof(fftwf_complex) * fft_size);
            memset(fftw_out, 0, sizeof(fftwf_complex) * fft_size);

            // Output buffer
            fft_input_buffer = dsp::create_volk_buffer<complex_t>(fft_size);
            fft_output_buffer = dsp::create_volk_buffer<float>(fft_size);

            reshape_buffer_size = std::max<int>(dsp::STREAM_BUFFER_SIZE, rbuffer_rate * 10); // TODOREWORK
            fft_reshape_buffer = dsp::create_volk_buffer<complex_t>(reshape_buffer_size);
            in_reshape_buffer = 0;

            fft_mutex.unlock();
        }

        void FFTPanBlock::destroy_fft()
        {
            fftwf_free(fftw_in);
            fftwf_free(fftw_out);
            fftwf_destroy_plan(fftw_plan);
            volk_free(fft_input_buffer);
            volk_free(fft_output_buffer);
            volk_free(fft_reshape_buffer);
        }

        bool FFTPanBlock::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                inputs[0].fifo->free(iblk);
                return true;
            }

            int nsamples = iblk.size;

            fft_mutex.lock();

            if (in_reshape_buffer + nsamples < reshape_buffer_size)
            {
                memcpy(&fft_reshape_buffer[in_reshape_buffer], iblk.getSamples<complex_t>(), nsamples * sizeof(complex_t));
                in_reshape_buffer += nsamples;
            }
            inputs[0].fifo->free(iblk);

            if (in_reshape_buffer > rbuffer_rate)
            {
                int pos_in_buffer = 0;
                while (in_reshape_buffer - pos_in_buffer > rbuffer_rate)
                {
                    memcpy(fft_input_buffer, &fft_reshape_buffer[pos_in_buffer], rbuffer_size * sizeof(complex_t));
                    pos_in_buffer += rbuffer_rate;

                    complex_t *buffer_ptr = fft_input_buffer;

                    volk_32fc_32f_multiply_32fc((lv_32fc_t *)fftw_in, (lv_32fc_t *)buffer_ptr, fft_taps.data(), rbuffer_size);

                    fftwf_execute(fftw_plan);

                    volk_32fc_s32f_power_spectrum_32f(fft_output_buffer, (lv_32fc_t *)fftw_out, fft_size, fft_size);

                    if (avg_num < 1)
                        avg_num = 1;
                    float avg_rate = 1.0 / avg_num;

                    for (int i = 0; i < fft_size; i++)
                        output_fft_buff[i] = output_fft_buff[i] * (1.0f - avg_rate) + fft_output_buffer[i] * avg_rate;

                    on_fft(output_fft_buff);
                }

                if (pos_in_buffer < in_reshape_buffer)
                {
                    memmove(fft_reshape_buffer, &fft_reshape_buffer[pos_in_buffer], (in_reshape_buffer - pos_in_buffer) * sizeof(complex_t));
                    in_reshape_buffer -= pos_in_buffer;
                }
            }

            fft_mutex.unlock();
            return false;
        }
    } // namespace ndsp
} // namespace satdump