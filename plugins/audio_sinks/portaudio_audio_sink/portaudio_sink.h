#pragma once

#include <portaudio.h>
#include "common/audio/audio_sink.h"
#include <mutex>
#include <vector>

class PortAudioSink : public audio::AudioSink
{
private:
    int d_samplerate;

    std::mutex audio_mtx;
    std::vector<int16_t> audio_buff;

    static int audio_callback(const void *input, void *output, unsigned long frameCount,
                              const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags,
                              void *userData);

    PaStream *stream;

public:
    PortAudioSink();
    ~PortAudioSink();
    void set_samplerate(int samplerate);
    void start();
    void stop();

    void push_samples(int16_t *samples, int nsamples);

public:
    static std::string getID() { return "portaudio"; }
    static std::shared_ptr<audio::AudioSink> getInstance() { return std::make_shared<PortAudioSink>(); }
};