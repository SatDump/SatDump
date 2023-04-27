#pragma once

#include <aaudio/AAudio.h>
#include "common/audio/audio_sink.h"
#include <mutex>
#include <vector>

class AAudioSink : public audio::AudioSink
{
private:
    int d_samplerate;

    std::mutex audio_mtx;
    std::vector<int16_t> audio_buff;

    bool is_open;
    AAudioStream *stream;

    static void error_callback(AAudioStream *stream, void *userData, aaudio_result_t error);

public:
    AAudioSink();
    ~AAudioSink();
    void set_samplerate(int samplerate);
    void start();
    void stop();

    void push_samples(int16_t *samples, int nsamples);

public:
    static std::string getID() { return "rtaudio"; }
    static std::shared_ptr<audio::AudioSink> getInstance() { return std::make_shared<AAudioSink>(); }
};