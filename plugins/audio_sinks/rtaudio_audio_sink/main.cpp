#include "core/plugin.h"
#include "logger.h"
#include "rtaudio_sink.h"

class RtAudioAudioSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "rtaudio_audio_sink";
    }

    void init()
    {
        satdump::eventBus->register_handler<audio::RegisterAudioSinkEvent>(registerSinks);
    }

    static void registerSinks(const audio::RegisterAudioSinkEvent &evt)
    {
        evt.sink_registry.emplace(RtAudioSink::getID(), RtAudioSink::getInstance);
    }
};

PLUGIN_LOADER(RtAudioAudioSupport)