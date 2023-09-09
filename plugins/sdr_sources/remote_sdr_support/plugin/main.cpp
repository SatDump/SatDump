#include "core/plugin.h"
#include "logger.h"
#include "remote_source.h"

class RemoteSDRSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "remote_sdr_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({RemoteSource::getID(), {RemoteSource::getInstance, RemoteSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(RemoteSDRSupport)