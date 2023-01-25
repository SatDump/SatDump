#include "core/plugin.h"
#include "logger.h"
#include "rtaudio_sdr.h"

class RtlSdrSDRSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "rtaudio_sdr_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({RtAudioSource::getID(), {RtAudioSource::getInstance, RtAudioSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(RtlSdrSDRSupport)