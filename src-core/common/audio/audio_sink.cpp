#include "audio_sink.h"
#include "core/plugin.h"
#include "logger.h"

namespace audio
{
    std::map<std::string, std::function<std::shared_ptr<AudioSink>()>> sink_registry;

    AudioSink::AudioSink()
    {
    }

    class DummyAudioSink : public AudioSink
    {
    public:
        void set_samplerate(int) {}
        void start() {}
        void stop() {}

        void push_samples(int16_t *, int) {}

    public:
        static std::string getID() { return "dummy"; }
        static std::shared_ptr<AudioSink> getInstance() { return std::make_shared<DummyAudioSink>(); }
    };

    void registerSinks()
    {
        satdump::eventBus->fire_event<RegisterAudioSinkEvent>({sink_registry});
    }

    std::shared_ptr<AudioSink> get_default_sink()
    {
        if (sink_registry.count("portaudio"))
        {
            logger->info("Using PortAudio Sink");
            return sink_registry["portaudio"]();
        }
        else if (sink_registry.count("rtaudio"))
        {
            logger->info("Using RTAudio Sink");
            return sink_registry["rtaudio"]();
        }
        else
            return std::make_shared<DummyAudioSink>();
    }

    bool has_sink()
    {
        return sink_registry.size() > 0;
    }
}