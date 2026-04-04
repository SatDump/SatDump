#include "bladerf_dev.h"
#include "common/dsp/complex.h"
#include <bladeRF2.h>
#include <libbladeRF.h>
#include <logger.h>
#include <string>

namespace satdump
{
    namespace ndsp
    {
        void BladeRFDevBlock::mainThread_rx()
        {
            int16_t *sample_buffer;
            bladerf_metadata meta;

            {
                sample_buffer = new int16_t[sample_buffer_size * 2 * rx_ch_number];

                while (thread_should_run_rx)
                {
                    if (bladerf_sync_rx(bladerf_dev_obj, sample_buffer, sample_buffer_size, &meta, 4000) != 0)
                        break;

                    if (rx_ch_number == 1)
                    {
                        DSPBuffer oblk = outputs[0].fifo->newBufferSamples(sample_buffer_size, sizeof(complex_t));
                        if (is_8bit)
                            volk_8i_s32f_convert_32f((float *)oblk.getSamples<complex_t>(), (int8_t *)sample_buffer, 127.0f, sample_buffer_size * 2);
                        else
                            volk_16i_s32f_convert_32f((float *)oblk.getSamples<complex_t>(), sample_buffer, 4096.0f, sample_buffer_size * 2);
                        oblk.size = meta.actual_count;
                        outputs[0].fifo->wait_enqueue(oblk);
                    }
                    else if (rx_ch_number == 2)
                    {
                        int nsam = meta.actual_count / 2;
                        for (int s = 0; s < 2; s++)
                        {
                            DSPBuffer oblk = outputs[s].fifo->newBufferSamples(sample_buffer_size, sizeof(complex_t));
                            auto c = oblk.getSamples<complex_t>();
                            if (is_8bit)
                            {
                                for (int i = 0; i < nsam; i++)
                                {
                                    c[i].real = ((int8_t *)sample_buffer)[(i * 2 + s) * 2 + 0] / 127.0f;
                                    c[i].imag = ((int8_t *)sample_buffer)[(i * 2 + s) * 2 + 1] / 127.0f;
                                }
                            }
                            else
                            {
                                for (int i = 0; i < nsam; i++)
                                {
                                    c[i].real = sample_buffer[(i * 2 + s) * 2 + 0] / 4096.0f;
                                    c[i].imag = sample_buffer[(i * 2 + s) * 2 + 1] / 4096.0f;
                                }
                            }
                            oblk.size = nsam;
                            outputs[s].fifo->wait_enqueue(oblk);
                        }
                    }
                    else
                        throw satdump_exception("BladeRX RX can't have more than 2 channels!");
                }

                delete[] sample_buffer;
            }
        }

        void BladeRFDevBlock::mainThread_tx()
        {
            int16_t *sample_buffer;
            bladerf_metadata meta;

            {
                sample_buffer = new int16_t[sample_buffer_size * 2 * tx_ch_number];

                while (1)
                {
                    if (tx_ch_number == 1)
                    {
                        DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

                        if (iblk.isTerminator())
                        {
                            inputs[0].fifo->free(iblk);
                            break;
                        }

                        if (is_8bit)
                            volk_32f_s32f_convert_8i((int8_t *)sample_buffer, (float *)iblk.getSamples<complex_t>(), 127.0f, iblk.size * 2);
                        else
                            volk_32f_s32f_convert_16i(sample_buffer, (float *)iblk.getSamples<complex_t>(), 4096.0f, iblk.size * 2);
                        if (bladerf_sync_tx(bladerf_dev_obj, sample_buffer, iblk.size, &meta, 4000) != 0)
                            break;

                        inputs[0].fifo->free(iblk);
                    }
                    else
                        throw satdump_exception("BladeRX TX can't have more than one channel (YET)!");
                }

                delete[] sample_buffer;
            }
        }
    } // namespace ndsp
} // namespace satdump