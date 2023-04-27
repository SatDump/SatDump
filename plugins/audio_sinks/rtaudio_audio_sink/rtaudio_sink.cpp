#include "rtaudio_sink.h"
#include <cstring>
#include "logger.h"

int RtAudioSink::audio_callback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
                                double streamTime, RtAudioStreamStatus status, void *userData)
{
    RtAudioSink *tthis = (RtAudioSink *)userData;
    int16_t *buffer = (int16_t *)outputBuffer;

    tthis->audio_mtx.lock();
    unsigned int in_vec = tthis->audio_buff.size();
    tthis->audio_mtx.unlock();

    if (in_vec > nBufferFrames)
    {
        tthis->audio_mtx.lock();
        memcpy(buffer, tthis->audio_buff.data(), nBufferFrames * sizeof(int16_t));
        tthis->audio_buff.erase(tthis->audio_buff.begin(), tthis->audio_buff.begin() + nBufferFrames);
        tthis->audio_mtx.unlock();
    }
    else
    {
        memset(buffer, 0, nBufferFrames * sizeof(int16_t));
    }

    return 0;
}

RtAudioSink::RtAudioSink()
{
}

RtAudioSink::~RtAudioSink()
{
}

void RtAudioSink::set_samplerate(int samplerate)
{
    d_samplerate = samplerate;
}

void RtAudioSink::start()
{
    if (rt_dac.getDeviceCount() < 1)
        logger->error("No audio devices found!");

    rt_parameters.deviceId = rt_dac.getDefaultOutputDevice();
    rt_parameters.nChannels = 1;
    rt_parameters.firstChannel = 0;
    unsigned int sampleRate = d_samplerate;
    unsigned int bufferFrames = 256; // 256 sample frames
    try
    {
        rt_dac.openStream(&rt_parameters, NULL, RTAUDIO_SINT16,
                          sampleRate, &bufferFrames, &RtAudioSink::audio_callback, (void *)this);
        rt_dac.startStream();
    }
    catch (std::exception &e)
    {
        logger->error(e.what());
    }
}

void RtAudioSink::stop()
{
    rt_dac.stopStream();
    rt_dac.closeStream();
}

void RtAudioSink::push_samples(int16_t *samples, int nsamples)
{
    audio_mtx.lock();
    audio_buff.insert(audio_buff.end(), samples, samples + nsamples);
    audio_mtx.unlock();
}