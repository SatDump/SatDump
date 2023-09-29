#include "core/plugin.h"
#include "logger.h"
#include "udp_source.h"

class UDPSourceSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "udp_source_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({UDPSource::getID(), {UDPSource::getInstance, UDPSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(UDPSourceSupport)