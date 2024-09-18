#include "core/plugin.h"
#include "logger.h"
#include "net_source.h"

class NetSourceSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "network_source_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({NetSource::getID(), {NetSource::getInstance, NetSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(NetSourceSupport)