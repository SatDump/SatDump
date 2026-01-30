#include "portaudio_sink.h"
#include "common/dsp/buffer.h"
#include "common/dsp/io/baseband_type.h"
#include "core/config.h"
#include "core/exception.h"
#include "dsp/block.h"
#include "logger.h"
#include <complex.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <volk/volk_malloc.h>

namespace satdump
{
    namespace ndsp
    {
        PortAudioSinkBlock::PortAudioSinkBlock() : Block("portaudio_sink_f", {{"in", DSP_SAMPLE_TYPE_F32}}, {}) {}

        PortAudioSinkBlock::~PortAudioSinkBlock() {}

        void PortAudioSinkBlock::start()
        {
            PaError err = Pa_Initialize();
            if (err != paNoError)
                logger->error("Couldn't init PortAudio! %s", Pa_GetErrorText(err));

            err = Pa_OpenDefaultStream(&stream,                             //
                                       0,                                   //
                                       1,                                   //
                                       paInt16,                             //
                                       samplerate,                          //
                                       256,                                 //
                                       &PortAudioSinkBlock::audio_callback, //
                                       this);
            if (err != paNoError)
                logger->error("Couldn't open PortAudio! %s", Pa_GetErrorText(err));

            err = Pa_StartStream(stream);
            if (err != paNoError)
                logger->error("Couldn't start PortAudio! %s", Pa_GetErrorText(err));

            Block::start();
        }

        void PortAudioSinkBlock::stop(bool stop_now)
        {
            Block::stop(stop_now);

            if (stream != nullptr)
            {
                PaError err = Pa_StopStream(stream);
                if (err != paNoError)
                    logger->error("Couldn't stop PortAudio! %s", Pa_GetErrorText(err));

                err = Pa_CloseStream(stream);
                if (err != paNoError)
                    logger->error("Couldn't close PortAudio! %s", Pa_GetErrorText(err));

                err = Pa_Terminate();
                if (err != paNoError)
                    logger->error("Couldn't terminate PortAudio! %s", Pa_GetErrorText(err));
            }
        }

        int PortAudioSinkBlock::audio_callback(const void *, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *, PaStreamCallbackFlags, void *userData)
        {
            PortAudioSinkBlock *tthis = (PortAudioSinkBlock *)userData;
            int16_t *buffer = (int16_t *)output;

            tthis->audio_mtx.lock();
            unsigned int in_vec = tthis->audio_buff.size();
            tthis->audio_mtx.unlock();

            if (in_vec > frameCount)
            {
                tthis->audio_mtx.lock();
                memcpy(buffer, tthis->audio_buff.data(), frameCount * sizeof(int16_t));
                tthis->audio_buff.erase(tthis->audio_buff.begin(), tthis->audio_buff.begin() + frameCount);
                tthis->audio_mtx.unlock();
            }
            else
            {
                memset(buffer, 0, frameCount * sizeof(int16_t));
            }

            return 0;
        }

        bool PortAudioSinkBlock::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                inputs[0].fifo->free(iblk);
                return true;
            }

            int nsam = iblk.size;
            float *ibuf = iblk.getSamples<float>();

        retry:
            audio_mtx.lock();
#if 1
            if (audio_buff.size() > samplerate * 0.2)
            {
                audio_mtx.unlock();
                goto retry;
            }
#else
            if (audio_buff.size() < samplerate * 1)
#endif
            {
                std::vector<int16_t> buf_resamp_out(nsam);
                volk_32f_s32f_convert_16i(buf_resamp_out.data(), ibuf, nsam, 32767);
                audio_buff.insert(audio_buff.end(), buf_resamp_out.data(), buf_resamp_out.data() + nsam);
                audio_mtx.unlock();
            }

            inputs[0].fifo->free(iblk);

            return false;
        }
    } // namespace ndsp
} // namespace satdump