#include "litexm2sdr_dev.h"

namespace satdump
{
    namespace ndsp
    {
        void LiteXM2SDRDevBlock::mainThread_rx()
        {
            int16_t *sample_buffer;

            {
                logger->info("Streaming RX!");

                sample_buffer = (int16_t *)m2sdr_alloc_buffer(M2SDR_FORMAT_SC16_Q11, samples_per_buf);

                while (thread_should_run_rx)
                {
                    int ret = m2sdr_sync_rx(litexm2sdr_dev_obj, sample_buffer, samples_per_buf, NULL, 0);
                    if (ret != 0)
                    {
                        logger->error("Error streaming %d!", ret);
                        break;
                    }

                    if (!is_dual_ch && rx_ch_number == 1)
                    {
                        DSPBuffer oblk = outputs[0].fifo->newBufferSamples(samples_per_buf, sizeof(complex_t));
                        if (is_8bit)
                            volk_8i_s32f_convert_32f((float *)oblk.getSamples<complex_t>(), (int8_t *)sample_buffer, 127.0f, samples_per_buf * 2);
                        else
                            volk_16i_s32f_convert_32f((float *)oblk.getSamples<complex_t>(), sample_buffer, 4095.0f, samples_per_buf * 2);

                        oblk.size = samples_per_buf;
                        outputs[0].fifo->wait_enqueue(oblk);
                    }
                    else if (is_dual_ch && rx_ch_number == 1)
                    {
                        DSPBuffer oblk = outputs[0].fifo->newBufferSamples(samples_per_buf, sizeof(complex_t));
                        if (is_8bit)
                            volk_8i_s32f_convert_32f((float *)oblk.getSamples<complex_t>(), (int8_t *)sample_buffer, 127.0f, samples_per_buf * 2);
                        else
                            volk_16i_s32f_convert_32f((float *)oblk.getSamples<complex_t>(), sample_buffer, 4095.0f, samples_per_buf * 2);

                        for (int i = 0; i < samples_per_buf / 2; i++)
                            oblk.getSamples<complex_t>()[i] = oblk.getSamples<complex_t>()[i * 2];

                        oblk.size = samples_per_buf / 2;
                        outputs[0].fifo->wait_enqueue(oblk);
                    }
                    else if (rx_ch_number == 2)
                    {
                        int nsam = /*meta.actual_count*/ samples_per_buf / 2;
                        for (int s = 0; s < 2; s++)
                        {
                            DSPBuffer oblk = outputs[s].fifo->newBufferSamples(samples_per_buf, sizeof(complex_t));
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
                        throw satdump_exception("LiteXM2SDR RX can't have more than 2 channels!");
                }

                logger->info("Exited Streaming RX!");

                delete[] sample_buffer;
            }
        }

        void LiteXM2SDRDevBlock::mainThread_tx()
        {
            m2sdr_metadata meta;

            volk::vector<int16_t> wip_buffer;

            {
                while (1)
                {
                    if (!is_dual_ch && tx_ch_number == 1)
                    {
                        DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

                        if (iblk.isTerminator())
                        {
                            inputs[0].fifo->free(iblk);
                            break;
                        }

                        if (is_8bit) {}
                        else
                        {
                            for (int i = 0; i < iblk.size; i++)
                            {
                                wip_buffer.push_back(iblk.getSamples<complex_t>()[i].real * 4095.f);
                                wip_buffer.push_back(iblk.getSamples<complex_t>()[i].imag * 4095.f);
                            }
                        }

                        inputs[0].fifo->free(iblk);

                        while (wip_buffer.size() >= (samples_per_buf * 2))
                        {
                            if (m2sdr_sync_tx(litexm2sdr_dev_obj, wip_buffer.data(), samples_per_buf, &meta, 4000) != 0)
                                logger->error("TX Stream error!");
                            wip_buffer.erase(wip_buffer.begin(), wip_buffer.begin() + (samples_per_buf * 2));
                        }
                    }
                    else if (is_dual_ch && tx_ch_number == 1)
                    {
                        DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

                        if (iblk.isTerminator())
                        {
                            inputs[0].fifo->free(iblk);
                            break;
                        }

                        if (is_8bit) {}
                        else
                        {
                            for (int i = 0; i < iblk.size; i++)
                            {
                                wip_buffer.push_back(iblk.getSamples<complex_t>()[i].real * 4096.f);
                                wip_buffer.push_back(iblk.getSamples<complex_t>()[i].imag * 4096.f);
                                wip_buffer.push_back(0);
                                wip_buffer.push_back(0);
                            }
                        }

                        inputs[0].fifo->free(iblk);

                        while (wip_buffer.size() >= (samples_per_buf * 2))
                        {
                            if (m2sdr_sync_tx(litexm2sdr_dev_obj, wip_buffer.data(), samples_per_buf, &meta, 4000) != 0)
                                logger->error("TX Stream error!");
                            wip_buffer.erase(wip_buffer.begin(), wip_buffer.begin() + (samples_per_buf * 2));
                        }
                    }
                    else
                        throw satdump_exception("LiteXM2Sdr TX can't have more than one channel (YET)!");
                }
            }
        }
    } // namespace ndsp
} // namespace satdump