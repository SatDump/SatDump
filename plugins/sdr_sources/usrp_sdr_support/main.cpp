#include "core/plugin.h"
#include "logger.h"
#include "usrp_sdr.h"

class USRPSDRSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "usrp_sdr_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({USRPSource::getID(), {USRPSource::getInstance, USRPSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(USRPSDRSupport)