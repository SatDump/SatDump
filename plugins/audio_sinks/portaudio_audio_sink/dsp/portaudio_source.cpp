#include "portaudio_source.h"
#include "logger.h"
#include <portaudio.h>

namespace satdump
{
    namespace ndsp
    {
        PortAudioSourceBlock::PortAudioSourceBlock() : Block("portaudio_source_f", {}, {{"out", DSP_SAMPLE_TYPE_F32}}) {}

        PortAudioSourceBlock::~PortAudioSourceBlock() {}

        void PortAudioSourceBlock::start()
        {
            PaError err = Pa_Initialize();
            if (err != paNoError)
                logger->error("Couldn't init PortAudio! %s", Pa_GetErrorText(err));

            err = Pa_OpenDefaultStream(&stream,       //
                                       1,             //
                                       0,             //
                                       paFloat32,     //
                                       samplerate,    //
                                       d_buffer_size, //
                                       NULL,          //
                                       this);
            if (err != paNoError)
                logger->error("Couldn't open PortAudio! %s", Pa_GetErrorText(err));

            err = Pa_StartStream(stream);
            if (err != paNoError)
                logger->error("Couldn't start PortAudio! %s", Pa_GetErrorText(err));

            Block::start();
        }

        void PortAudioSourceBlock::stop(bool stop_now, bool force)
        {
            Block::stop(stop_now, force);

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

        bool PortAudioSourceBlock::work()
        {
            if (!work_should_exit)
            {
                auto oblk = outputs[0].fifo->newBufferSamples(d_buffer_size, sizeof(float));
                float *obuf = oblk.getSamples<float>();

                PaError err = Pa_ReadStream(stream, obuf, d_buffer_size);
                if (err != paNoError)
                    logger->error("Couldn't read input from PortAudio! %s", Pa_GetErrorText(err));

                oblk.size = d_buffer_size;
                outputs[0].fifo->wait_enqueue(oblk);
            }
            else
            {
                outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                return true;
            }

            return false;
        }
    } // namespace ndsp
} // namespace satdump