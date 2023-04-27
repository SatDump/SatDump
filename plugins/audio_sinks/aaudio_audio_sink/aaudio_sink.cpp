#include "aaudio_sink.h"
#include <cstring>
#include "logger.h"

AAudioSink::AAudioSink()
{
    is_open = false;
}

AAudioSink::~AAudioSink()
{
}

void AAudioSink::set_samplerate(int samplerate)
{
    d_samplerate = samplerate;
}

void AAudioSink::error_callback(AAudioStream *stream, void *userData, aaudio_result_t error)
{
    if (error == AAUDIO_ERROR_DISCONNECTED)
    {
        AAudioSink *v = (AAudioSink *)userData;
        v->stop();
        v->start();
    }
}

void AAudioSink::start()
{
    audio_mtx.lock();
    AAudioStreamBuilder *builder;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);

    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setSampleRate(builder, d_samplerate);
    AAudioStreamBuilder_setChannelCount(builder, 1);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, 256);
    AAudioStreamBuilder_setErrorCallback(builder, &AAudioSink::error_callback, this);

    AAudioStreamBuilder_openStream(builder, &stream);
    AAudioStream_requestStart(stream);

    AAudioStreamBuilder_delete(builder);

    is_open = true;
    audio_mtx.unlock();
}

void AAudioSink::stop()
{
    audio_mtx.lock();
    AAudioStream_requestStop(stream);
    AAudioStream_close(stream);
    is_open = false;
    audio_mtx.unlock();
}

void AAudioSink::push_samples(int16_t *samples, int nsamples)
{
    audio_mtx.lock();
    if (is_open)
    {
        audio_buff.insert(audio_buff.end(), samples, samples + nsamples);

        while (audio_buff.size() > 256)
        {
            AAudioStream_write(stream, audio_buff.data(), 256, 100000000);
            audio_buff.erase(audio_buff.begin(), audio_buff.begin() + 256);
        }
    }
    audio_mtx.unlock();
}