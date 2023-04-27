#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <functional>
#include <memory>

namespace audio
{
    class AudioSink
    {
    public:
        AudioSink();

        virtual void set_samplerate(int samplerate) = 0;
        virtual void start() = 0;
        virtual void stop() = 0;

        virtual void push_samples(int16_t *samples, int nsamples) = 0;

    public:
        static std::string getID();
        static std::shared_ptr<AudioSink> getInstance();
    };

    extern std::map<std::string, std::function<std::shared_ptr<AudioSink>()>> sink_registry;

    // Event where modules are registered, so plugins can load theirs
    struct RegisterAudioSinkEvent
    {
        std::map<std::string, std::function<std::shared_ptr<AudioSink>()>> &sink_registry;
    };

    void registerSinks();
    std::shared_ptr<AudioSink> get_default_sink();
    bool has_sink();
}