#include "portaudio_sink.h"
#include <cstring>
#include "logger.h"

int PortAudioSink::audio_callback(const void *input, void *output, unsigned long frameCount,
                                  const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags,
                                  void *userData)
{
    PortAudioSink *tthis = (PortAudioSink *)userData;
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

PortAudioSink::PortAudioSink()
{
    PaError err = Pa_Initialize();
    if (err != paNoError)
        logger->error("Couldn't init PortAudio! %s", Pa_GetErrorText(err));
}

PortAudioSink::~PortAudioSink()
{
    PaError err = Pa_Terminate();
    if (err != paNoError)
        logger->error("Couldn't terminate PortAudio! %s", Pa_GetErrorText(err));
}

void PortAudioSink::set_samplerate(int samplerate)
{
    d_samplerate = samplerate;
    d_final_samplerate = 48e3; // For now...
}

void PortAudioSink::start()
{
    PaError err;
    err = Pa_OpenDefaultStream(&stream,
                               0,
                               1,
                               paInt16,
                               d_final_samplerate,
                               256,
                               &PortAudioSink::audio_callback,
                               this);
    if (err != paNoError)
        logger->error("Couldn't open PortAudio! %s", Pa_GetErrorText(err));

    err = Pa_StartStream(stream);
    if (err != paNoError)
        logger->error("Couldn't start PortAudio! %s", Pa_GetErrorText(err));
}

void PortAudioSink::stop()
{
    PaError err = Pa_StopStream(stream);
    if (err != paNoError)
        logger->error("Couldn't stop PortAudio! %s", Pa_GetErrorText(err));

    err = Pa_CloseStream(stream);
    if (err != paNoError)
        logger->error("Couldn't close PortAudio! %s", Pa_GetErrorText(err));
}

void PortAudioSink::push_samples(int16_t *samples, int nsamples)
{
    audio_mtx.lock();
    std::vector<int16_t> buf_resamp_out(nsamples * (double(d_final_samplerate) / double(d_samplerate)) * 10);
    int nout = resample_s16(samples, buf_resamp_out.data(), d_samplerate, d_final_samplerate, nsamples, 1);
    audio_buff.insert(audio_buff.end(), buf_resamp_out.data(), buf_resamp_out.data() + nout);
    audio_mtx.unlock();
}