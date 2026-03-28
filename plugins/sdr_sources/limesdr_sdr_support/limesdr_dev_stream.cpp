#include "limesdr_dev.h"
#include <lime/ConnectionHandle.h>
#include <lime/ConnectionRegistry.h>
#include <lime/Streamer.h>
#include <lime/lms7_device.h>
#include <logger.h>
#include <string>

namespace satdump
{
    namespace ndsp
    {
        void LimeSDRDevBlock::mainThread_rx()
        {
            int sample_buffer_size = std::min<int>(samplerate / 250, 1e6);

            while (thread_should_run_rx)
            {
                int ch = 0;
                for (auto &s : rx_streams)
                {
                    lime::StreamChannel::Metadata md;

                    DSPBuffer oblk = outputs[ch].fifo->newBufferSamples(sample_buffer_size, sizeof(complex_t));

                    int v = s->Read(oblk.getSamples<complex_t>(), sample_buffer_size, &md);

                    oblk.size = v;
                    if (v > 0)
                        outputs[ch].fifo->wait_enqueue(oblk);
                    else
                        outputs[ch].fifo->free(oblk);
                    ch++;
                }
            }
        }

        void LimeSDRDevBlock::mainThread_tx()
        {
            bool terminate = false;

            while (1)
            {
                if (terminate)
                    break;

                int ch = 0;
                for (auto &s : tx_streams)
                {
                    DSPBuffer iblk = inputs[ch].fifo->wait_dequeue();

                    if (iblk.isTerminator())
                    {
                        inputs[ch].fifo->free(iblk);
                        terminate = true;
                        break;
                    }

                    lime::StreamChannel::Metadata md;

                    int status = s->Write(iblk.getSamples<complex_t>(), iblk.size, &md);
                    if (status == 0)
                        logger->error("LimeSDR TX Timeout!");
                    else if (status <= 0)
                        logger->error("LimeSDR TX Error! %d", status);
                    else if (status != iblk.size)
                        logger->error("LimeSDR Stream dropped!");

                    inputs[ch].fifo->free(iblk);
                    ch++;
                }
            }
        }
    } // namespace ndsp
} // namespace satdump