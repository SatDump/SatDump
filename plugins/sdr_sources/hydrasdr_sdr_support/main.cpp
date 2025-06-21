#include "core/plugin.h"
#include "hydrasdr_sdr.h"
#include "logger.h"

class HydraDRSupport : public satdump::Plugin
{
public:
    std::string getID() { return "hydrasdr_sdr_support"; }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({HydraSDRSource::getID(), {HydraSDRSource::getInstance, HydraSDRSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(HydraDRSupport)