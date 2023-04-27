#include "core/plugin.h"
#include "logger.h"
#include "aaudio_sink.h"

class AAudioAudioSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "aaudio_audio_sink";
    }

    void init()
    {
        satdump::eventBus->register_handler<audio::RegisterAudioSinkEvent>(registerSinks);
    }

    static void registerSinks(const audio::RegisterAudioSinkEvent &evt)
    {
        evt.sink_registry.emplace(AAudioSink::getID(), AAudioSink::getInstance);
    }
};

PLUGIN_LOADER(AAudioAudioSupport)