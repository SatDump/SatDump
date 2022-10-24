#include "core/plugin.h"
#include "logger.h"
#include "rtltcp.h"

class RTLTCPSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "rtltcp_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({RTLTCPSource::getID(), {RTLTCPSource::getInstance, RTLTCPSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(RTLTCPSupport)