#include "fft_ddc.h"
#include "common/dsp/buffer.h"
#include "dsp/block_helpers.h"
#include "dsp/ddc/fft_helper.h"
#include <complex.h>
#include <fftw3.h>
#include <memory>
#include <type_traits>

namespace satdump
{
    namespace ndsp
    {
        FFTDDCBlock::FFTDDCBlock() : Block("fft_ddc_c", {{"in", getTypeSampleType<complex_t>()}}, {}) {}

        FFTDDCBlock::~FFTDDCBlock() {}

        void FFTDDCBlock::init()
        {
            d_ntaps = d_filter_m * 2 + 1;
            d_fftsize = (int)(2 * pow(2.0, ceil(log(double(d_ntaps)) / log(2.0))));
            d_nsamples = d_fftsize - d_ntaps + 1;

            printf("TAPS %d FFT %d SAMP %d\n", d_ntaps, d_fftsize, d_nsamples);

            fft_fwd = std::make_shared<FFTHelper>(d_fftsize, true);

            // Init buffer
            buffer.resize(8192);
            buffer_size = buffer.size();
            in_buffer = 0;

            // Init everything else
            for (auto &o : outputs)
                ((IOInfo *)o.blkdata.get())->initFFTFilter(this);
        }

        bool FFTDDCBlock::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                if (iblk.terminatorShouldPropagate())
                {
                    vfos_mtx.lock();
                    for (auto &o : outputs)
                    {
                        IOInfo *i = ((IOInfo *)o.blkdata.get());
                        if (i->forward_terminator)
                            o.fifo->wait_enqueue(o.fifo->newBufferTerminator());
                    }
                    vfos_mtx.unlock();
                }
                inputs[0].fifo->free(iblk);
                return true;
            }

            if (needs_reinit)
            {
                needs_reinit = false;
                init();
            }

            // Simply copy those over
            bool has_actual_vfos = false;

            vfos_mtx.lock();

            for (auto &o : outputs)
            {
                IOInfo *i = ((IOInfo *)o.blkdata.get());

                if (i->frequency == 0 && i->bandwidth == 0 && i->decimation <= 1)
                {
                    DSPBuffer oblk = o.fifo->newBufferSamples(d_nsamples, sizeof(complex_t));
                    memcpy(oblk.getSamples<complex_t>(), iblk.getSamples<complex_t>(), iblk.size * sizeof(complex_t));
                    oblk.size = iblk.size;
                    o.fifo->wait_enqueue(oblk);
                }
                else
                {
                    has_actual_vfos = true;
                }
            }

            vfos_mtx.unlock();

            if (!has_actual_vfos)
            {
                inputs[0].fifo->free(iblk);
                return false;
            }

            // Now actual VFOs
            int nsamples = iblk.size;

            // Automatically grow buffer
            if (buffer_size < in_buffer + nsamples)
            {
                buffer.resize((buffer_size + nsamples) * 2);
                buffer_size = buffer.size();
            }

            memcpy(&buffer[in_buffer], iblk.getSamples<complex_t>(), nsamples * sizeof(complex_t));
            in_buffer += nsamples;

            int consumed = 0;

            for (int i = 0; (i + d_nsamples) < in_buffer; i += d_nsamples)
            {
                vfos_mtx.lock();

                memcpy(fft_fwd->fft_in, &buffer[i], d_nsamples * sizeof(complex_t));
                consumed += d_nsamples;

                for (int j = d_nsamples; j < d_fftsize; j++)
                    ((complex_t *)fft_fwd->fft_in)[j] = complex_t(0.0f, 0.0f);

                fft_fwd->execute();

                for (auto &o : outputs)
                {
                    IOInfo *i = ((IOInfo *)o.blkdata.get());

                    // TODO by type?
                    if (i->frequency == 0 && i->bandwidth == 0 && i->decimation <= 1)
                        continue;

                    volk_32fc_x2_multiply_32fc_a((lv_32fc_t *)i->fft_inv->fft_in, (lv_32fc_t *)fft_fwd->fft_out, (lv_32fc_t *)i->ffttaps_buffer.data(), d_fftsize);
                    i->fft_inv->execute();

                    for (int j = 0; j < (d_ntaps - 1); j++)
                        ((complex_t *)i->fft_inv->fft_out)[j] += i->tail[j];

                    DSPBuffer oblk = o.fifo->newBufferSamples(d_nsamples, sizeof(complex_t));
                    complex_t *obuf = oblk.getSamples<complex_t>();

                    memcpy(obuf, (complex_t *)i->fft_inv->fft_out, d_nsamples * sizeof(complex_t));
                    memcpy(i->tail.data(), (complex_t *)i->fft_inv->fft_out + d_nsamples, (d_ntaps - 1) * sizeof(complex_t));

#if VOLK_VERSION >= 030100
                    volk_32fc_s32fc_x2_rotator2_32fc((lv_32fc_t *)obuf, (const lv_32fc_t *)obuf, (lv_32fc_t *)&i->phase_delta, (lv_32fc_t *)&i->phase, d_nsamples);
#else
                    volk_32fc_s32fc_x2_rotator_32fc((lv_32fc_t *)obuf, (const lv_32fc_t *)obuf, i->phase_delta, (lv_32fc_t *)&i->phase, d_nsamples);
#endif

                    int size_out = 0;
                    for (int p = 0; p < d_nsamples; p++)
                    {
                        i->sample_index = (i->sample_index + 1) % i->decimation;
                        if (i->sample_index == 0)
                        {
                            obuf[size_out++] = obuf[p];
                            //    printf(" %d,%d \n", p, size_out);
                        }
                    }

                    oblk.size = size_out; // d_nsamples;
                    o.fifo->wait_enqueue(oblk);
                }

                vfos_mtx.unlock();
            }

            memmove(&buffer[0], &buffer[consumed], (in_buffer - consumed) * sizeof(complex_t));
            in_buffer -= consumed;

            inputs[0].fifo->free(iblk);

            return false;
        }
    } // namespace ndsp
} // namespace satdump