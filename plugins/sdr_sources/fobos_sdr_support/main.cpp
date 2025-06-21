#include "core/plugin.h"
#include "fobos_sdr.h"
#include "logger.h"

class FobosSDRSupport : public satdump::Plugin
{
public:
    std::string getID() { return "fobos_sdr_support"; }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);     
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({FobosSource::getID(), {FobosSource::getInstance, FobosSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(FobosSDRSupport)