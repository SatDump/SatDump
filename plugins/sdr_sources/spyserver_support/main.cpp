#include "core/plugin.h"
#include "logger.h"
#include "spyserver.h"

class SpyServerSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "spyserver_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({SpyServerSource::getID(), {SpyServerSource::getInstance, SpyServerSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(SpyServerSupport)