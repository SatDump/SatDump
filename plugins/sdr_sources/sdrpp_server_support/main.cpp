#include "core/plugin.h"
#include "logger.h"
#include "sdrpp_server.h"

class SDRPPServerSupport : public satdump::Plugin
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
        evt.dsp_sources_registry.insert({SDRPPServerSource::getID(), {SDRPPServerSource::getInstance, SDRPPServerSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(SDRPPServerSupport)