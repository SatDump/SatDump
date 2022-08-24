#include "core/plugin.h"
#include "logger.h"
#include "mirisdr_sdr.h"

class MiriSdrSDRSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "mirisdr_sdr_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({MiriSdrSource::getID(), {MiriSdrSource::getInstance, MiriSdrSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(MiriSdrSDRSupport)