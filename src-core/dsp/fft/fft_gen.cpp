#include "fft_gen.h"

/*
 * @brief This is Júlia's monstrosity
 * to generate floats that will then be
 * use to make images from FFTs
 * TODOREWORK
 */

namespace satdump
{
    namespace ndsp
    {
        FFTGenBlock::FFTGenBlock() : Block("fft_gen_cf", {{"in", DSP_SAMPLE_TYPE_CF32}}, {})
        {
            output_buff = new float[dsp::STREAM_BUFFER_SIZE]; // TODOREWORK
            d_fft_gen_end = false;
        }

        FFTGenBlock::~FFTGenBlock()
        {
            if (output_buffer != nullptr)
                destroy_fft();
            delete[] output_buff;
        }

        void FFTGenBlock::set_fft_gen_settings(int fft_size, int fft_overlap, uint32_t block_size)
        {
            fft_gen_mutex.lock();

            d_fft_size = fft_size;
            d_n = fft_overlap;
            d_block_size = block_size;
            d_fft_taps_size = d_fft_size / d_n;

            if (output_buffer != nullptr)
                destroy_fft();

            rbuffer_rate = d_block_size;
            rbuffer_size = d_fft_taps_size;
            rbuffer_skip = rbuffer_rate - rbuffer_size;

            fft_taps.resize(rbuffer_size);
            for (int i = 0; i < rbuffer_size; i++)
                fft_taps[i] = dsp::window::nuttall(i, rbuffer_size - 1) * ((i % 2) ? 1.0f : -1.0f);

            fftw_in = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * d_fft_size);
            fftw_out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * d_fft_size);
            fftw_plan = fftwf_plan_dft_1d(d_fft_size, fftw_in, fftw_out, FFTW_FORWARD, FFTW_ESTIMATE);

            memset(fftw_in, 0, sizeof(fftwf_complex) * d_fft_size);
            memset(fftw_out, 0, sizeof(fftwf_complex) * d_fft_size);

            input_buffer = dsp::create_volk_buffer<complex_t>(d_fft_size);
            output_buffer = dsp::create_volk_buffer<float>(d_fft_size);

            reshape_buffer_size = std::max<int>(dsp::STREAM_BUFFER_SIZE, rbuffer_rate * 10); // TODOREWORK
            fft_reshape_buffer = dsp::create_volk_buffer<complex_t>(reshape_buffer_size);
            in_reshape_buffer = 0;

            fft_gen_mutex.unlock();
        }

        void FFTGenBlock::destroy_fft()
        {
            d_fft_gen_end = true;
            fftwf_free(fftw_in);
            fftwf_free(fftw_out);
            fftwf_destroy_plan(fftw_plan);
            volk_free(input_buffer);
            volk_free(output_buffer);
            volk_free(fft_reshape_buffer);
        }

        bool FFTGenBlock::work()
        {
            DSPBuffer iblk;
            inputs[0].fifo->wait_dequeue(iblk);

            if (iblk.isTerminator())
            {
                iblk.free();
                return true;
            }

            uint32_t nsamples = iblk.size;

            fft_gen_mutex.lock();

            if (in_reshape_buffer + nsamples < reshape_buffer_size)
            {
                memcpy(&fft_reshape_buffer[in_reshape_buffer], iblk.getSamples<complex_t>(), nsamples * sizeof(complex_t));
                in_reshape_buffer += nsamples;
            }
            iblk.free();

            if (in_reshape_buffer > rbuffer_rate)
            {
                int pos_in_buffer = 0;
                while (in_reshape_buffer - pos_in_buffer > rbuffer_rate)
                {
                    memcpy(input_buffer, &fft_reshape_buffer[pos_in_buffer], rbuffer_size * sizeof(complex_t));
                    pos_in_buffer += rbuffer_rate;

                    complex_t *buffer_ptr = input_buffer;

                    volk_32fc_32f_multiply_32fc((lv_32fc_t *)fftw_in, (lv_32fc_t *)buffer_ptr, fft_taps.data(), rbuffer_size);
                    fftwf_execute(fftw_plan);
                    volk_32fc_s32f_power_spectrum_32f(output_buffer, (lv_32fc_t *)fftw_out, d_fft_size, d_fft_size);

                    float avg_rate = 1.0 / 10;

                    for (int i = 0; i < d_fft_size; i++)
                        output_buff[i] = output_buff[i] * (1.0f - avg_rate) + output_buffer[i] * avg_rate;

                    on_fft_gen_floats(output_buff);
                }
                if (pos_in_buffer < in_reshape_buffer)
                {
                    memmove(fft_reshape_buffer, &fft_reshape_buffer[pos_in_buffer], (in_reshape_buffer - pos_in_buffer) * sizeof(complex_t));
                    in_reshape_buffer -= pos_in_buffer;
                }
            }

            fft_gen_mutex.unlock();
            return false;
        }

    } // namespace ndsp
} // namespace satdump
